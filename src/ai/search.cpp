#include "search.h"

#include <stdexcept>
#include <algorithm>
#include <thread>

#include "../movegen.h"

namespace lunachess::ai {

/**
 * Pseudo-exception to be thrown when the search time is up.
 */
class TimeUp {
};

constexpr int CHECK_TIME_NODE_INTERVAL = 4096;

void AlphaBetaSearcher::checkIfSearchIsOver() {
    if (m_LastResults.visitedNodes % CHECK_TIME_NODE_INTERVAL == 0 &&
        (m_TimeManager.timeIsUp()
         || m_ShouldStop)) {
        throw TimeUp();
    }
}

int AlphaBetaSearcher::quiesce(int ply, int alpha, int beta) {
    m_LastResults.visitedNodes++;

    checkIfSearchIsOver();

    int standPat = m_Eval->evaluate(m_Pos);

    if (standPat >= beta) {
        // Fail high
        return beta;
    }

    if (standPat > alpha) {
        // Before we search, if the standPat is higher than alpha then it means
        // that not performing a noisy move is best (so far).
        alpha = standPat;
    }

    MoveList moves;
    int moveCount = m_MvFactory.generateNoisyMoves(moves, m_Pos, ply);

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
        // No material delta could improve our position enough, we can
        // perform some pruning.
        return alpha;
    }
    // #----------------------------------------

    for (int i = 0; i < moveCount; ++i) {
        Move move = moves[i];
        if (move.getType() == MT_SIMPLE_CAPTURE &&
            !posutils::hasGoodSEE(m_Pos, move)) {
            // The result of the exchange series will always result in
            // material loss after this capture, prune it.
            continue;
        }

        m_Pos.makeMove(move);
        int score = -quiesce(ply + 1, -beta, -alpha);
        m_Pos.undoMove();

        if (score >= beta) {
            // Fail high
            return beta;
        }

        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}

int AlphaBetaSearcher::alphaBeta(int depth, int ply,
                                 int alpha, int beta,
                                 bool nullMoveAllowed,
                                 MoveList* searchMoves) {
    m_LastResults.visitedNodes++;

    bool isRoot = ply == 0;
    if (m_Pos.isDraw(1) && !isRoot) {
        // Position is a draw (or has repeated already), return draw score.
        return m_Eval->getDrawScore();
    }

    checkIfSearchIsOver();

    // Setup some important variables
    int staticEval; // Used for some pruning/reduction techniques
    const int originalDepth = depth;
    const int originalAlpha = alpha;

    // Before we start the search, probe the transposition table.
    // If we had already searched this same position before but with
    // equal or higher depth, we remove ourselves the burden of researching it
    // and reuse previous results.
    // 
    // If we find data in the TT searched at lowe depth, however, searching 
    // will still be necessary. Although we can still use the found data
    // as a heuristic to accelerate the proccess of searching. For instance,
    // there is a great chance that the best move at this position on a depth d
    // is also the best move at d - 1 depth. Thus, we can search the best move
    // at depth d - 1 first.
    
    Move hashMove = MOVE_INVALID; // Move extracted from TT
    ui64 posKey = m_Pos.getZobrist();
    TranspositionTable::Entry ttEntry = {};
    ttEntry.zobristKey = posKey;
    ttEntry.move = MOVE_INVALID;

    bool foundInTT = m_TT.probe(posKey, ttEntry);
    if (foundInTT) {
        staticEval = ttEntry.staticEval;

        // We don't care about the TT entry if its hash move is not within
        // the scope of our searchMoves list.
        if (searchMoves == nullptr || searchMoves->contains(ttEntry.move)) {
            hashMove = ttEntry.move;
            if (ttEntry.depth >= depth) {
                if (ttEntry.type == TranspositionTable::EXACT) {
                    if (searchMoves == nullptr || searchMoves->contains(ttEntry.move)) {
                        // Only use transposition table exact moves if we're either not using
                        // an external provided search moves list or the one we're using
                        // includes the move found in the TT.
                        return ttEntry.score;
                    }
                } else if (ttEntry.type == TranspositionTable::LOWERBOUND) {
                    alpha = std::max(alpha, ttEntry.score);
                } else if (ttEntry.type == TranspositionTable::UPPERBOUND) {
                    beta = std::min(beta, ttEntry.score);
                }

                if (alpha >= beta) {
                    return ttEntry.score;
                }
            }
        }
    }
    else {
        // No TT entry found, we need to compute the static eval here.
        staticEval = m_Eval->evaluate(m_Pos);
    }
    // #----------------------------------------

    if (depth <= 0) {
        return quiesce(ply, alpha, beta);
    }

    bool isCheck = m_Pos.isCheck();
    if (!isCheck) {
        // Only decrease depth if we're not currently in check
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
        m_MvFactory.generateMoves(moves, m_Pos, ply, hashMove);
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

        // #----------------------------------------
        // # LATE MOVE REDUCTIONS
        // #----------------------------------------
        bool reduced = false;
        constexpr int LMR_START_IDX = 4;
        if (d >= 4 &&
            !isCheck &&
            move.is<MTM_QUIET>() &&
            !m_MvFactory.isKillerMove(move, ply) &&
            i >= LMR_START_IDX &&
            (foundInTT && ttEntry.type == TranspositionTable::UPPERBOUND)) {
            d--;
            reduced = true;
        }

        // #----------------------------------------

        m_Pos.makeMove(move);

        int score = -alphaBeta(d, ply + 1, -beta, -alpha);
        if (score > alpha && reduced) {
            // Research
            score = alphaBeta(depth, ply + 1, -beta, -alpha);
        }

        m_Pos.undoMove();

        if (score >= beta) {
            // Beta cutoff
            alpha = beta;

            bestMoveIdx = i;

            if (move.is<MTM_QUIET>()) {
                m_MvFactory.storeHistory(move, d);
            }
            break;
        }
        if (score > alpha) {
            alpha = score;
            bestMoveIdx = i;
        }
    }

    // Store search data in transposition table
    Move bestMove = (*searchMoves)[bestMoveIdx];

    if (alpha <= originalAlpha) {
        // Fail low
        ttEntry.type = TranspositionTable::UPPERBOUND;
    }
    else if (alpha >= beta) {
        // Fail high
        if (bestMove.is<MTM_QUIET>()) {
            m_MvFactory.storeKillerMove(bestMove, ply);
        }
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

    try {
        // Reset everything
        m_Searching = true;
        m_ShouldStop = false;
        m_MvFactory.resetHistory();

        // Setup variables
        m_Pos = pos;
        int drawScore = m_Eval->getDrawScore();
        int maxDepth = std::min(MAX_SEARCH_DEPTH, settings.maxDepth);

        // Generate and order all moves
        MoveList moves;
        m_MvFactory.generateMoves(moves, m_Pos, 0, MOVE_INVALID);

        // If no moves were generated, position is a stalemate or checkmate.
        if (moves.size() == 0) {
            int score = m_Pos.isCheck()
                        ? -MATE_SCORE // Checkmate
                        : drawScore;  // Stalemate

            m_LastResults.bestScore = score;
            m_LastResults.bestMove = MOVE_INVALID;
            m_Searching = false;

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

        // Notify the time manager that we're starting a search
        m_TimeManager.start(settings.ourTimeControl);

        // Perform iterative deepening, starting at depth 1
        for (int depth = 1; depth <= maxDepth; depth++) {
            if (m_TimeManager.timeIsUp() || m_ShouldStop) {
                break;
            }

            // Caller might be asking for a multi-pv search. Search the number of pvs requested
            // or until all moves were searched
            for (int multipv = 0; multipv < settings.multiPvCount && moves.size() > 0; ++multipv) {
                if (m_TimeManager.timeIsUp() || m_ShouldStop) {
                    break;
                }

                // Prepare results object for this search
                m_LastResults.visitedNodes = 0;
                m_LastResults.currDepthStart = Clock::now();

                // Perform the search
                try {
                    TranspositionTable::Entry ttEntry;

                    int score = alphaBeta(depth, 0, -HIGH_BETA, HIGH_BETA, false, &moves);

                    m_TT.probe(m_Pos, ttEntry);

                    if (multipv == 0) {
                        // multipv == 0 means that this is the true principal variation.
                        // The score and move found in this pv search are the best for the
                        // position being searched.
                        m_LastResults.bestScore = score;
                        m_LastResults.bestMove = ttEntry.move;
                        m_LastResults.searchedDepth = depth;
                    }
                    moves.remove(ttEntry.move);


                    // We finished the search on this variation at this depth.
                    // Now, properly fill the searched variation object for this pv
                    // in the results object.
                    auto& pv = m_LastResults.searchedVariations[multipv];
                    pv.score = ttEntry.score;
                    pv.type = TranspositionTable::EXACT;

                    // Check for moves on the PV in transposition table
                    pv.moves.clear();
                    while (m_TT.probe(m_Pos, ttEntry)) {
                        pv.moves.push_back(ttEntry.move);
                        m_Pos.makeMove(ttEntry.move);

                        if (m_Pos.isRepetitionDraw()) {
                            break;
                        }
                    }

                    // Undo all moves
                    for (int i = 0; i < pv.moves.size(); ++i) {
                        m_Pos.undoMove();
                    }

                    // When using multipvs, we need to clear the entry of the initial
                    // search position.
                    if (settings.multiPvCount > 1) {
                        m_TT.remove(m_Pos);
                    }

                    // Notify handler
                    if (settings.onPvFinish != nullptr) {
                        settings.onPvFinish(m_LastResults, multipv);
                    }
                }
                catch (const TimeUp&) {
                    // Time over
                    break;
                }
            }

            // Re-generate moves list with new ordering
            moves.clear();

            m_MvFactory.generateMoves(moves, m_Pos, 0, m_LastResults.bestMove);
            filterMoves(moves, settings.moveFilter);

            m_TimeManager.onNewDepth(m_LastResults);
            if (settings.onDepthFinish != nullptr) {
                settings.onDepthFinish(m_LastResults);
            }
        }

        m_Searching = false;

        return m_LastResults;
    }
    catch (const std::exception& e) {
        m_Searching = false;
        std::cerr << e.what() << std::endl;
        throw;
    }
}

}