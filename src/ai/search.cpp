#include "search.h"

#include <stdexcept>
#include <algorithm>
#include <thread>
#include "../movegen.h"

namespace lunachess::ai {

void MoveSearcher::extractVariation(Move move, SearchedVariation& var) {
    var.moves.clear();

    // Add the initial move
    m_Pos.makeMove(move);
    var.moves.push_back(move);

    // Search the transposition table for the moves that came afterwards
    TranspositionTable::Entry entry;

    bool firstEntry = true;

    while (m_TT.tryGet(m_Pos.getZobrist(), entry)) {
        if (firstEntry) {
            firstEntry = false;
            var.score = -entry.score;
            var.type = entry.type;
        }

        var.moves.push_back(entry.move);
        m_Pos.makeMove(entry.move);

        // Prevent infinite loops by checking for a draw by repetition
        if (m_Pos.isRepetitionDraw(1)) {
            break;
        }
    }

    // Reset the position to the state before extracting the variation
    for (int i = 0; i < var.moves.size(); ++i) {
        // Undo all moves
        m_Pos.undoMove();
    }
}

bool MoveSearcher::updateResults() {
    // Sort variations before updating lastSearchResults
    std::sort(m_LastResults.searchedVariations.begin(), m_LastResults.searchedVariations.end(),
          [](const SearchedVariation& a, const SearchedVariation& b) {
              if (a.type == TranspositionTable::EXACT && b.type != TranspositionTable::EXACT) {
                  return true;
              }
              if (a.type != TranspositionTable::EXACT && b.type == TranspositionTable::EXACT) {
                  return false;
              }
              if (a.score > b.score) {
                  return true;
              }
              if (a.score < b.score) {
                  return false;
              }
              return false;
          });
    bool ret = m_Handler(m_LastResults);

    return ret;
}

int MoveSearcher::generateAndOrderMovesQuiesce(MoveList& ml, int ply) {
    int initialCount = ml.size();

    m_MvFactory.generateNoisyMoves(ml, m_Pos, ply);

    return ml.size() - initialCount;
}

int MoveSearcher::generateAndOrderMoves(MoveList& ml, int ply, Move pvMove) {
    int initialCount = ml.size();

    m_MvFactory.generateMoves(ml, m_Pos, ply, pvMove);

    return ml.size() - initialCount;
}

/**
 * Pseudo-exception to be thrown when the search time is up.
 */
class TimeUpException : public std::exception {
};

constexpr int CHECK_TIME_NODE_INTERVAL = 4096;

int MoveSearcher::quiesce(int ply, int alpha, int beta) {
    m_LastResults.visitedNodes++;

    if (m_LastResults.visitedNodes % CHECK_TIME_NODE_INTERVAL == 0 &&
        (m_TimeManager.timeIsUp()
         || m_ShouldStop)) {
        throw TimeUpException();
    }

    int standPat = m_Eval->evaluate(m_Pos);

    if (standPat >= beta) {
        return beta;
    }

    if (standPat > alpha) {
        alpha = standPat;
    }

    MoveList moves;
    int moveCount = generateAndOrderMovesQuiesce(moves, ply);

    // #----------------------------------------
    // # DELTA PRUNING
    // #----------------------------------------
    int bigDelta = 10000;

    // Check whether we have pawns that can be promoted
    Bitboard promoters = m_Pos.getColorToMove() == CL_WHITE
            ? bbs::getRankBitboard(RANK_7)
            : bbs::getRankBitboard(RANK_2);
    promoters &= m_Pos.getBitboard(Piece(m_Pos.getColorToMove(), PT_PAWN));
    if (promoters > 0) {
        bigDelta += 9000;
    }

    if (standPat < alpha - bigDelta) {
        return alpha;
    }
    // #----------------------------------------

    for (int i = 0; i < moveCount; ++i) {
        Move move = moves[i];

        m_Pos.makeMove(move);
        int score = -quiesce(ply + 1, -beta, -alpha);
        m_Pos.undoMove();

        if (score >= beta) {
            return beta;
        }

        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}

int MoveSearcher::searchInternal(int depth, int ply, int alpha, int beta, bool nullMoveAllowed) {
    m_LastResults.visitedNodes++;

    if (m_Pos.isDraw(1)) {
        return m_Eval->getDrawScore();
    }

    if (m_LastResults.visitedNodes % CHECK_TIME_NODE_INTERVAL == 0 &&
            (m_TimeManager.timeIsUp()
            || m_ShouldStop)) {
        throw TimeUpException();
    }

    bool isRoot = ply == 0;
    const int originalDepth = depth;
    const int originalAlpha = alpha;
    int staticEval;
    Move hashMove = MOVE_INVALID;

    // #----------------------------------------
    // # TRANSPOSITION TABLE PROBING
    // #----------------------------------------
    // We might have already found the best move for this position in
    // a search with equal or higher depth. Look for it in the TT.
    ui64 posKey = m_Pos.getZobrist();
    TranspositionTable::Entry ttEntry = {};
    ttEntry.move = MOVE_INVALID;

    bool foundInTT = m_TT.tryGet(posKey, ttEntry);
    if (foundInTT) {
        if (ttEntry.depth >= depth) {
            if (ttEntry.type == TranspositionTable::EXACT) {
                return ttEntry.score;
            } else if (ttEntry.type == TranspositionTable::LOWERBOUND) {
                alpha = std::max(alpha, ttEntry.score);
            } else if (ttEntry.type == TranspositionTable::UPPERBOUND) {
                beta = std::min(beta, ttEntry.score);
            }

            if (alpha >= beta) {
                return ttEntry.score;
            }
        }
        staticEval = ttEntry.staticEval;
    }
    else {
        staticEval = m_Eval->evaluate(m_Pos);
    }
    hashMove = ttEntry.move;
    // #----------------------------------------

    if (depth <= 0) {
        return quiesce(ply, alpha, beta);
    }

    // #----------------------------------------
    // # CHECK EXTENSION
    // #----------------------------------------
    // Check extension -- do not decrease depth when searching for moves
    // in positions in check.
    bool isCheck = m_Pos.isCheck();
    if (!isCheck) {
        depth--;
    }
    // #----------------------------------------

    int drawScore = m_Eval->getDrawScore();

    // #----------------------------------------
    // # NULL MOVE PRUNING
    // #----------------------------------------
    // Prune if making a null move fails high
    constexpr int NULL_SEARCH_DEPTH_RED = 2;
    constexpr int NULL_SEARCH_MIN_DEPTH = NULL_SEARCH_DEPTH_RED + 1;
    constexpr int NULL_MOVE_MIN_PIECES = 4;

    if (nullMoveAllowed && !isCheck &&
        depth >= NULL_SEARCH_MIN_DEPTH &&
        m_Pos.getBitboard(Piece(m_Pos.getColorToMove(), PT_NONE)).count() > NULL_MOVE_MIN_PIECES) {

        // Null move pruning allowed
        m_Pos.makeNullMove();

        int score = -searchInternal(depth - NULL_SEARCH_DEPTH_RED, ply + 1, -beta, -beta + 1, false);
        if (score >= beta) {
            m_Pos.undoNullMove();
            return beta; // Prune
        }

        m_Pos.undoNullMove();

        // #----------------------------------------
        // # FAIL HIGH REDUCTIONS
        // #----------------------------------------
        constexpr int THREAT_LEVEL = 2500;
        constexpr int MIN_ADVANTAGE = 5000;
        constexpr int FAIL_HIGH_RED = 2;
        if (staticEval > MIN_ADVANTAGE &&
            score > staticEval - THREAT_LEVEL) {
            // We have a substantial advantage and, based on the
            // null move observation, the opponent didn't have any substantial
            // threats against us. We can reduce the search depth.
            depth -= FAIL_HIGH_RED;
        }
        // #----------------------------------------
    }
    // #----------------------------------------

    // Generate moves
    MoveList moves;
    int moveCount = generateAndOrderMoves(moves, ply, hashMove);
    if (moveCount == 0) {
        // Either stalemate or checkmate
        if (isCheck) {
            return -MATE_SCORE + ply;
        }
        return drawScore;
    }

    int bestMoveIdx = 0;

    // Finally, do the search

    // Setup principal variation
    Move* pvStart = m_PvIt;
    *m_PvIt++ = MOVE_INVALID;

    for (int i = 0; i < moveCount; ++i) {
        Move move = moves[i];

        int d = depth;

        // #----------------------------------------
        // # FUTILITY PRUNING
        // #----------------------------------------
        // Prune frontier/pre-frontier nodes with no chance of improving evaluation.
        constexpr int FUTILITY_MARGIN = 2500;
        if (!isCheck && !isRoot && move.is<MTM_QUIET>()) {
            if (d == 1 && (staticEval + FUTILITY_MARGIN) < alpha) {
                // Prune
                continue;
            }
            if (d == 2 && (staticEval + FUTILITY_MARGIN * 2) < alpha) {
                // Prune
                continue;
            }
        }
        // #----------------------------------------

        // #----------------------------------------
        // # LATE MOVE REDUCTIONS
        // #----------------------------------------
        constexpr int LMR_REDUCTION = 1;
        constexpr int LMR_MIN_DEPTH = LMR_REDUCTION + 1;
        if (foundInTT && d > LMR_MIN_DEPTH &&
            ttEntry.type == TranspositionTable::UPPERBOUND &&
            i > 4 &&
            !isCheck &&
            move.is<MTM_NOISY>()) {
            d -= LMR_REDUCTION;
        }
        // #----------------------------------------

        m_Pos.makeMove(move);
        int score = -searchInternal(d, ply + 1, -beta, -alpha);
        m_Pos.undoMove();

        if (score >= beta) {
            // Beta cutoff
            alpha = beta;

            bestMoveIdx = i;

            if (!move.is<MTM_NOISY>()) {
                m_MvFactory.storeHistory(move, d);
                m_MvFactory.storeKillerMove(move, ply);
            }
            break;
        }
        if (score > alpha) {
            alpha = score;
            bestMoveIdx = i;

            Move* p = m_PvIt;
            m_PvIt = pvStart;
            *m_PvIt++ = move;
            while ((*m_PvIt++ = *p++) != MOVE_INVALID);
        }
    }
    m_PvIt = pvStart;

    // Store search data in transposition table
    Move bestMove = moves[bestMoveIdx];
    if (alpha <= originalAlpha) {
        // Fail low
        ttEntry.type = TranspositionTable::UPPERBOUND;
    }
    else if (alpha >= beta) {
        // Fail high
        ttEntry.type = TranspositionTable::LOWERBOUND;
    }
    else {
        ttEntry.type = TranspositionTable::EXACT;
    }

    ttEntry.depth = originalDepth;
    ttEntry.move = bestMove;
    ttEntry.score = alpha;
    ttEntry.zobristKey = posKey;
    ttEntry.staticEval = staticEval;
    m_TT.maybeAdd(ttEntry);
    return alpha;
}

void MoveSearcher::search(const Position& pos, SearchResultsHandler handler, SearchSettings settings) {
    while (m_Searching);

    try {
        m_Searching = true;
        m_ShouldStop = false;
        m_PvIt = m_Pv.begin();

        std::fill(m_Pv.begin(), m_Pv.end(), MOVE_INVALID);

        TranspositionTable::Entry ttEntry;

        int drawScore = m_Eval->getDrawScore();

        m_Handler = handler;
        m_Pos = pos;

        // Generate and order all moves
        MoveList moves;

        Clock clock;
        m_LastResults.visitedNodes = 1;
        m_LastResults.searchStart = clock.now();

        // Last lastSearchResults could have been filled with a previous search
        m_LastResults.searchedVariations.clear();

        generateAndOrderMoves(moves, 0, MOVE_INVALID);

        // Add a variation object for each move being searched
        // The indexes of the generated moves list will match
        // the variations in this vector.
        m_LastResults.searchedVariations.resize(1);

        // Check if no legal moves (stalemate/checkmate)
        if (moves.size() == 0) {
            int score = m_Pos.isCheck()
                        ? -MATE_SCORE // Checkmate
                        : drawScore;  // Stalemate

            m_LastResults.bestScore = score;
            m_LastResults.bestMove = MOVE_INVALID;

            updateResults();
            return;
        }

        m_LastResults.bestMove = moves[0];

        int alpha = -HIGH_BETA;
        int beta = HIGH_BETA;
        int window = 9000;

        ui64 posKey = m_Pos.getZobrist();

        m_TimeManager.start(settings.ourTimeControl);

        // Perform iterative deepening, starting at depth 1
        for (int depth = 1; depth < MAX_SEARCH_DEPTH && !m_TimeManager.timeIsUp() && !m_ShouldStop; depth++) {
            m_LastResults.visitedNodes = 0;
            m_LastResults.currDepthStart = clock.now();

            // Perform the search
            bool mustResearch = false;
            try {
                do {
                    m_LastResults.bestScore = searchInternal(depth, 0, alpha, beta, false);

                    m_LastResults.bestMove = m_Pv[0];
                    m_LastResults.searchedDepth = depth;

                    if (!mustResearch) {
                        if (m_LastResults.bestScore == beta) {
                            mustResearch = true;
                            alpha = -HIGH_BETA;
                            beta = HIGH_BETA;
                        }
                    }
                    else {
                        mustResearch = false;
                    }
                } while (mustResearch);
            }
            catch (const TimeUpException &ex) {
                // Time over
                break;
            }

            auto& principalVariation = m_LastResults.searchedVariations[0];
            principalVariation.moves.clear();
            principalVariation.score = m_LastResults.bestScore;
            principalVariation.type = TranspositionTable::EXACT;
            for (auto move : m_Pv) {
                if (move == MOVE_INVALID) {
                    break;
                }
                principalVariation.moves.push_back(move);
            }

            if (updateResults()) {
                // Search stop requested.
                break;
            }

            moves.clear();

            movegen::generate(pos, moves);

            if (depth > 5) {
                window -= 1000 / (depth - 4);
                alpha = m_LastResults.bestScore - window;
                beta = m_LastResults.bestScore + window;
            } else {
                alpha = -HIGH_BETA;
                beta = HIGH_BETA;
            }
        }
    }
    catch (const std::exception& ex) {
    }
    m_Searching = false;
}

}