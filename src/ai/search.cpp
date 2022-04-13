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
class TimeUp {
};

constexpr int CHECK_TIME_NODE_INTERVAL = 4096;

int MoveSearcher::quiesce(int ply, int alpha, int beta) {
    m_LastResults.visitedNodes++;

    if (m_LastResults.visitedNodes % CHECK_TIME_NODE_INTERVAL == 0 &&
        (m_TimeManager.timeIsUp()
         || m_ShouldStop)) {
        throw TimeUp();
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
        if (move.getType() == MT_SIMPLE_CAPTURE &&
            !posutils::hasGoodSEE(m_Pos, move)) {
            continue;
        }

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

void MoveSearcher::pushMoveToPv(TPV::iterator& pvStart, Move move) {
    TPV::iterator p = m_PvIt;
    m_PvIt = pvStart;
    *m_PvIt++ = move;
    while ((*m_PvIt++ = *p++) != MOVE_INVALID);
}

int MoveSearcher::alphaBeta(int depth, int ply, int alpha, int beta, bool nullMoveAllowed, MoveList* searchMoves) {
    m_LastResults.visitedNodes++;

    if (m_Pos.isDraw(1)) {
        return m_Eval->getDrawScore();
    }

    if (m_LastResults.visitedNodes % CHECK_TIME_NODE_INTERVAL == 0 &&
            (m_TimeManager.timeIsUp()
            || m_ShouldStop)) {
        throw TimeUp();
    }

    bool isRoot = ply == 0;
    const int originalDepth = depth;
    const int originalAlpha = alpha;
    int staticEval;
    Move hashMove = MOVE_INVALID;

    // Setup principal variation
    TPV::iterator pvStart = m_PvIt;
    *m_PvIt++ = MOVE_INVALID;

    // #----------------------------------------
    // # TRANSPOSITION TABLE PROBING
    // #----------------------------------------
    // We might have already found the best move for this position in
    // a search with equal or higher depth. Look for it in the TT.
    ui64 posKey = m_Pos.getZobrist();
    TranspositionTable::Entry ttEntry = {};
    ttEntry.zobristKey = posKey;
    ttEntry.move = MOVE_INVALID;

    bool foundInTT = m_TT.tryGet(posKey, ttEntry);
    if (foundInTT) {
        hashMove = ttEntry.move;

        if (ttEntry.depth >= depth) {
            if (ttEntry.type == TranspositionTable::EXACT) {
                if (searchMoves == nullptr || searchMoves->contains(ttEntry.move)) {
                    // Only use transposition table exact moves if we're either not using
                    // an external provided search moves list or the one we're using
                    // includes the move found in the TT
                    pushMoveToPv(pvStart, ttEntry.move);
                    m_PvIt = pvStart;
                    return ttEntry.score;
                }
            } else if (ttEntry.type == TranspositionTable::LOWERBOUND) {
                alpha = std::max(alpha, ttEntry.score);
            } else if (ttEntry.type == TranspositionTable::UPPERBOUND) {
                beta = std::min(beta, ttEntry.score);
            }

            if (alpha >= beta) {
                m_PvIt = pvStart;
                return ttEntry.score;
            }
        }
        staticEval = ttEntry.staticEval;
    }
    else {
        staticEval = m_Eval->evaluate(m_Pos);
    }
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

        int score = -alphaBeta(depth - NULL_SEARCH_DEPTH_RED, ply + 1, -beta, -beta + 1, false);
        if (score >= beta) {
            m_Pos.undoNullMove();
            m_PvIt = pvStart;
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
    if (searchMoves == nullptr) {
        generateAndOrderMoves(moves, ply, hashMove);
        searchMoves = &moves;
    }
    if (searchMoves->size() == 0) {
        // Either stalemate or checkmate
        if (isCheck) {
            return -MATE_SCORE + ply;
        }
        return drawScore;
    }

    // Finally, do the search
    int bestMoveIdx = 0;

    bool canReduce = true;
    for (int i = 0; i < searchMoves->size(); ++i) {
        Move move = (*searchMoves)[i];

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


        m_Pos.makeMove(move);
        int score = -alphaBeta(d, ply + 1, -beta, -alpha);
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

            pushMoveToPv(pvStart, move);
        }
    }
    m_PvIt = pvStart;

    // Store search data in transposition table
    Move bestMove = (*searchMoves)[bestMoveIdx];

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

static void filterMoves(MoveList& ml, MoveSearchFilter filter) {
    for (int i = ml.size() - 1; i >= 0; i--) {
        auto move = ml[i];
        if (!filter(move)) {
            ml.removeAt(i);
        }
    }
}

void MoveSearcher::search(const Position& pos, SearchResultsHandler handler, SearchSettings settings) {
    while (m_Searching);

    try {
        m_Searching = true;
        m_ShouldStop = false;

        m_MvFactory.resetHistory();
        TranspositionTable::Entry ttEntry;

        int drawScore = m_Eval->getDrawScore();

        m_Handler = handler;
        m_Pos = pos;

        // Generate and order all moves
        MoveList moves;
        generateAndOrderMoves(moves, 0, MOVE_INVALID);

        // Filter out undesired moves
        if (settings.moveFilter != nullptr) {
            filterMoves(moves, settings.moveFilter);
        }

        m_LastResults.visitedNodes = 1;
        m_LastResults.searchStart = Clock::now();

        // Last lastSearchResults could have been filled with a previous search
        m_LastResults.searchedVariations.clear();

        // Check if no moves (stalemate/checkmate)
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

        ui64 posKey = m_Pos.getZobrist();

        m_TimeManager.start(settings.ourTimeControl);

        // Perform iterative deepening, starting at depth 1
        for (int depth = 1; depth < MAX_SEARCH_DEPTH && !m_TimeManager.timeIsUp() && !m_ShouldStop; depth++) {
            for (int multipv = 0; multipv < settings.multiPvCount; ++multipv) {
                std::fill(m_Pv.begin(), m_Pv.end(), MOVE_INVALID);
                m_PvIt = m_Pv.begin();

                m_LastResults.visitedNodes = 0;
                m_LastResults.currDepthStart = Clock::now();

                // Perform the search
                try {
                    int score = alphaBeta(depth, 0, alpha, beta, false, &moves);

                    if (multipv == 0) {
                        m_LastResults.bestScore = score;
                        m_LastResults.bestMove = m_Pv[0];
                        m_LastResults.searchedDepth = depth;
                    }

                    moves.remove(m_Pv[0]);
                }
                catch (const TimeUp &) {
                    // Time over
                    break;
                }

                auto& pv = m_LastResults.searchedVariations[multipv];
                pv.moves.clear();
                pv.score = m_LastResults.bestScore;
                pv.type = TranspositionTable::EXACT;
                for (auto move: m_Pv) {
                    if (move == MOVE_INVALID) {
                        break;
                    }
                    pv.moves.push_back(move);
                }

                if (updateResults()) {
                    // Search stop requested.
                    break;
                }
            }

            if (m_TimeManager.timeIsUp()) {
                break;
            }

            // Re-generate moves list with new ordering
            moves.clear();

            movegen::generate(pos, moves);
            if (settings.moveFilter != nullptr) {
                filterMoves(moves, settings.moveFilter);
            }
        }
    }
    catch (const std::exception& ex) {
    }
    m_Searching = false;
}

}