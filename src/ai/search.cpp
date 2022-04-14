#include "search.h"

#include <stdexcept>
#include <algorithm>
#include <thread>
#include "../movegen.h"

namespace lunachess::ai {

void SearchResults::sortVariations() {
    // Sort variations before updating lastSearchResults
    std::sort(searchedVariations.begin(), searchedVariations.end(),
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
}

int AlphaBetaSearcher::generateAndOrderMovesQuiesce(MoveList& ml, int ply) {
    int initialCount = ml.size();

    m_MvFactory.generateNoisyMoves(ml, m_Pos, ply);

    return ml.size() - initialCount;
}

int AlphaBetaSearcher::generateAndOrderMoves(MoveList& ml, int ply, Move pvMove) {
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

int AlphaBetaSearcher::quiesce(int ply, int alpha, int beta) {
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

void AlphaBetaSearcher::pushMoveToPv(TPV::iterator& pvStart, Move move) {
    auto p = m_PvIt;
    m_PvIt = pvStart;
    *m_PvIt++ = move;
    while ((*m_PvIt++ = *p++) != MOVE_INVALID);
}

int AlphaBetaSearcher::alphaBeta(int depth, int ply, int alpha, int beta, bool nullMoveAllowed, MoveList* searchMoves) {
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
    auto pvStart = m_PvIt;
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

static void filterMoves(MoveList& ml, std::function<bool(Move)> filter) {
    if (filter == nullptr) {
        return;
    }

    for (int i = ml.size() - 1; i >= 0; i--) {
        auto move = ml[i];
        if (!filter(move)) {
            ml.removeAt(i);
        }
    }
}

SearchResults AlphaBetaSearcher::search(const Position& pos, SearchSettings settings) {
    while (m_Searching); // Wait current search.

    // Reset everything
    m_Searching = true;
    m_ShouldStop = false;
    m_MvFactory.resetHistory();

    // Setup variables
    m_Pos = pos;
    int drawScore = m_Eval->getDrawScore();

    // Generate and order all moves
    MoveList moves;
    generateAndOrderMoves(moves, 0, MOVE_INVALID);

    // If no moves were generated, position is a stalemate or checkmate.
    if (moves.size() == 0) {
        int score = m_Pos.isCheck()
                    ? -MATE_SCORE // Checkmate
                    : drawScore;  // Stalemate

        m_LastResults.bestScore = score;
        m_LastResults.bestMove = MOVE_INVALID;

        return m_LastResults;
    }

    // Filter out undesired moves
    filterMoves(moves, settings.moveFilter);

    // Setup results object
    m_LastResults.visitedNodes = 1;
    m_LastResults.searchStart = Clock::now();

    // Last lastSearchResults could have been filled with a previous search
    m_LastResults.searchedVariations.clear();

    m_LastResults.bestMove = moves[0];
    m_LastResults.searchedVariations.resize(moves.size());

    int alpha = -HIGH_BETA;
    int beta = HIGH_BETA;

    // Notify the time manager that we're starting a search
    m_TimeManager.start(settings.ourTimeControl);

    // Perform iterative deepening, starting at depth 1
    for (int depth = 1; depth < MAX_SEARCH_DEPTH && !m_TimeManager.timeIsUp() && !m_ShouldStop; depth++) {
        if (m_TimeManager.timeIsUp() || m_ShouldStop) {
            break;
        }
        // Caller might be asking for a multi-pv search. Search the number of pvs requested
        // or until all moves were searched
        for (int multipv = 0; multipv < settings.multiPvCount && moves.size() > 0; ++multipv) {
            if (m_TimeManager.timeIsUp() || m_ShouldStop) {
                break;
            }

            // For each new pv to be searched, clear the previous pv array
            std::fill(m_Pv.begin(), m_Pv.end(), MOVE_INVALID);
            m_PvIt = m_Pv.begin();

            // Prepare results object for this search
            m_LastResults.visitedNodes = 0;
            m_LastResults.currDepthStart = Clock::now();

            // Perform the search
            try {
                int score = alphaBeta(depth, 0, alpha, beta, false, &moves);

                if (multipv == 0) {
                    // multipv == 0 means that this is the true principal variation.
                    // The score and move found in this pv search are the best for the
                    // position being searched.
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

            // We finished the search on this variation at this depth.
            // Now, properly fill the searched variation object for this pv
            // in the results object.
            auto& pv = m_LastResults.searchedVariations[multipv];
            pv.score = m_LastResults.bestScore;
            pv.type = TranspositionTable::EXACT;

            // Add the moves searched now, deleting previous moves stored
            // in the variation.
            pv.moves.clear();
            for (auto move: m_Pv) {
                if (move == MOVE_INVALID) {
                    // PV array is MOVE_INVALID terminated.
                    break;
                }
                pv.moves.push_back(move);
            }

            // Notify handler
            if (settings.onPvFinish != nullptr) {
                settings.onPvFinish(m_LastResults, multipv);
            }
        }

        // Re-generate moves list with new ordering
        moves.clear();
        generateAndOrderMoves(moves, 0, m_LastResults.bestMove);
        filterMoves(moves, settings.moveFilter);

        if (settings.onDepthFinish != nullptr) {
            settings.onDepthFinish(m_LastResults);
        }
    }

    m_Searching = false;

    return m_LastResults;
}

}