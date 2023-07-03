#include "search.h"

#include <stdexcept>
#include <algorithm>
#include <thread>

#include "../movegen.h"

namespace lunachess::ai {

/**
 * Pseudo-exception to be thrown when the search must be interrupted.
 */
class SearchInterrupt {
};

constexpr int CHECK_TIME_NODE_INTERVAL = 1024;

void AlphaBetaSearcher::interruptSearchIfNecessary() {
    if (m_LastResults.visitedNodes % CHECK_TIME_NODE_INTERVAL == 0 &&
        (m_TimeManager.timeIsUp()
         || m_ShouldStop)) {
        throw SearchInterrupt();
    }
}

int AlphaBetaSearcher::quiesce(int ply, int alpha, int beta) {
    const Position& pos = m_Eval->getPosition();
    m_LastResults.visitedNodes++;

    interruptSearchIfNecessary();

    int standPat = m_Eval->evaluate();

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
    int moveCount = m_MvFactory.generateNoisyMoves(moves, pos, ply);

    // #----------------------------------------
    // # DELTA PRUNING
    // #----------------------------------------
    int bigDelta = 10000;

    // Check whether we have pawns that can be promoted
    Bitboard promoters = pos.getColorToMove() == CL_WHITE
                         ? bbs::getRankBitboard(RANK_7)
                         : bbs::getRankBitboard(RANK_2);
    promoters &= pos.getBitboard(Piece(pos.getColorToMove(), PT_PAWN));
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
            !staticanalysis::hasGoodSEE(pos, move)) {
            // The result of the exchange series will always result in
            // material loss after this capture, prune it.
            continue;
        }

        m_Eval->makeMove(move);
        int score = -quiesce(ply + 1, -beta, -alpha);
        m_Eval->undoMove();

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

int AlphaBetaSearcher::negamax(int depth, int ply,
                               int alpha, int beta,
                               bool nullMoveAllowed,
                               MoveList *searchMoves) {
    const Position& pos = m_Eval->getPosition();
    bool isRoot = ply == 0;
    if (pos.isDraw() && !isRoot) {
        // Position is a draw, return draw score.
        return m_Eval->getDrawScore();
    }

    interruptSearchIfNecessary();

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
    ui64 posKey = pos.getZobrist();
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
                        m_LastResults.visitedNodes++;
                        return ttEntry.score;
                    }
                    else {
                        // This search uses an incompatible searchMoves list with the
                        // TT entry. Remove the entry.
                        m_TT.remove(posKey);
                    }
                }
                else if (ttEntry.type == TranspositionTable::LOWERBOUND) {
                    alpha = std::max(alpha, ttEntry.score);
                }
                else if (ttEntry.type == TranspositionTable::UPPERBOUND) {
                    beta = std::min(beta, ttEntry.score);
                }

                if (alpha >= beta) {
                    m_LastResults.visitedNodes++;
                    return ttEntry.score;
                }
            }
        }
    }
    else {
        // No TT entry found, we need to compute the static eval here.
        staticEval = m_Eval->evaluate();
    }
    // #----------------------------------------

    if (depth <= 0) {
        return quiesce(ply, alpha, beta);
    }
    m_LastResults.visitedNodes++;

    bool isCheck = pos.isCheck();
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
        pos.getBitboard(Piece(pos.getColorToMove(), PT_NONE)).count() > NULL_MOVE_MIN_PIECES) {

        // Null move pruning allowed
        m_Eval->makeNullMove();

        int score = -negamax(depth - NULL_SEARCH_DEPTH_RED, ply + 1, -beta, -beta + 1, false);
        if (score >= beta) {
            m_Eval->undoNullMove();
            return beta; // Prune
        }

        m_Eval->undoNullMove();
    }
    // #----------------------------------------

    // Generate moves
    MoveList moves;
    if (searchMoves == nullptr) {
        m_MvFactory.generateMoves(moves, pos, ply, hashMove);
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
    bool searchPv = true;
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
        m_Eval->makeMove(move);

        int score;
        if (searchPv) {
            // Perform PVS. First move of the list is always PVS.
            score = -negamax(d, ply + 1, -beta, -alpha);
        }
        else {
            // #----------------------------------------
            // # LATE MOVE REDUCTIONS
            // #----------------------------------------
            constexpr int LMR_START_IDX = 2;
            if (d >= 2 &&
                !isCheck &&
                i >= LMR_START_IDX &&
                move.is<MTM_QUIET>()) {
                d -= 2;
            }
            // #----------------------------------------

            // Perform a ZWS. Searches after the first move are performed
            // with a null window. If the search fails high, do a re-search
            // with the full window.
            score = -negamax(d, ply + 1, -alpha - 1, -alpha);
            if (score > alpha) {
                score = -negamax(depth, ply + 1, -beta, -alpha);
            }
        }

        m_Eval->undoMove();

        if (score >= beta) {
            // Beta cutoff
            alpha = beta;

            bestMoveIdx = i;

            if (move.is<MTM_QUIET>()) {
                m_MvFactory.storeHistory(move, d);
                m_MvFactory.storeKillerMove(move, ply);
            }
            break;
        }
        if (score > alpha) {
            alpha = score;
            bestMoveIdx = i;
        }
        searchPv = false;
    }

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

static void filterMoves(MoveList &ml, std::function<bool(Move)> filter) {
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

SearchResults AlphaBetaSearcher::search(const Position &argPos, SearchSettings settings) {
    while (m_Searching); // Wait current search.

    try {
        // Reset everything
        m_Searching = true;
        m_ShouldStop = false;
        m_MvFactory.resetHistory();

        // Setup variables
        m_Eval->setPosition(argPos);
        const Position& pos = m_Eval->getPosition();
        int drawScore = m_Eval->getDrawScore();
        int maxDepth = std::min(MAX_SEARCH_DEPTH, settings.maxDepth);

        // Generate and order all moves
        MoveList moves;
        m_MvFactory.generateMoves(moves, pos, 0, MOVE_INVALID);

        // If no moves were generated, position is a stalemate or checkmate.
        if (moves.size() == 0) {
            int score = pos.isCheck()
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

        // Last search results could have been filled with a previous search
        m_LastResults.searchedVariations.clear();

        m_LastResults.bestMove = moves[0];
        m_LastResults.searchedVariations.resize(moves.size());

        // Notify the time manager that we're starting a search
        m_TimeManager.start(settings.ourTimeControl);

        // Clear transposition table for new use
        m_TT.clear();

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
                m_LastResults.currDepthStart = Clock::now();

                // Perform the search
                try {
                    TranspositionTable::Entry ttEntry;

                    constexpr int ASPIRATION_WINDOWS_MIN_DEPTH = 3;
                    constexpr int MAX_ASPIRATION_ITERATIONS = 4;

                    int alpha;
                    double alphaDelta = 1;
                    int beta;
                    double betaDelta = 1;

                    int score;
                    int lastScore = m_LastResults.bestScore;
                    if (depth < ASPIRATION_WINDOWS_MIN_DEPTH) {
                        // By default, perform a full window search
                        alpha = -HIGH_BETA;
                        beta = HIGH_BETA;
                    }
                    else {
                        // From depth ASPIRATION_WINDOWS_MIN_DEPTH onwards, try to use aspiration windows based
                        // on the last search.
                        alpha = lastScore - 500;
                        beta = lastScore + 500;
                    }

                    for (int aspirationIt = 0; aspirationIt <= MAX_ASPIRATION_ITERATIONS; ++aspirationIt) {
                        if (aspirationIt == MAX_ASPIRATION_ITERATIONS) {
                            // We tried many aspiration windows that didn't work, let's go with
                            // the full window.
                            alpha = -HIGH_BETA;
                            beta = HIGH_BETA;
                        }

                        score = negamax(depth, 0, alpha, beta, false, &moves);
                        if (score <= alpha) {
                            // Fail low, widen lower bound.
                            alpha -= static_cast<int>(alphaDelta * 1000);
                            alphaDelta -= std::pow(alphaDelta + 0.6, alphaDelta);
                        }
                        else if (score >= beta) {
                            // Fail high, increase lower bound
                            beta += static_cast<int>(betaDelta * 1000);
                            betaDelta += std::pow(betaDelta + 0.6, betaDelta);
                        }
                        else {
                            break;
                        }
                    }

                    m_TT.probe(pos, ttEntry);

                    if (multipv == 0) {
                        // multipv == 0 means that this is the true principal variation.
                        // The score and move found in this pv search are the best for the
                        // position being searched.
                        m_LastResults.bestScore = score;
                        if (ttEntry.move != MOVE_INVALID) {
                            m_LastResults.bestMove = ttEntry.move;
                        }
                        m_LastResults.searchedDepth = depth;
                    }
                    moves.remove(ttEntry.move);


                    // We finished the search on this variation at this depth.
                    // Now, properly fill the searched variation object for this pv
                    // in the results object.
                    auto &pv = m_LastResults.searchedVariations[multipv];
                    pv.score = ttEntry.score;
                    pv.type = TranspositionTable::EXACT;

                    // Check for moves on the PV in transposition table
                    pv.moves.clear();
                    while (m_TT.probe(pos, ttEntry) && ttEntry.move != MOVE_INVALID) {
                        pv.moves.push_back(ttEntry.move);
                        m_Eval->makeMove(ttEntry.move);

                        if (pos.isRepetitionDraw()) {
                            break;
                        }
                    }

                    // Undo all moves
                    for (int i = 0; i < pv.moves.size(); ++i) {
                        m_Eval->undoMove();
                    }

                    // When using multi PVs, we need to clear the entry of the initial
                    // search position.
                    if (settings.multiPvCount > 1) {
                        m_TT.remove(pos);
                    }

                    // Notify handler
                    if (settings.onPvFinish != nullptr) {
                        settings.onPvFinish(m_LastResults, multipv);
                    }
                }
                catch (const SearchInterrupt &) {
                    // Time over
                    break;
                }
            }

            // Re-generate moves list with new ordering
            moves.clear();

            m_MvFactory.generateMoves(moves, pos, 0, m_LastResults.bestMove);
            filterMoves(moves, settings.moveFilter);

            m_TimeManager.onNewDepth(m_LastResults);
            if (settings.onDepthFinish != nullptr) {
                settings.onDepthFinish(m_LastResults);
            }
        }

        m_Searching = false;

        return m_LastResults;
    }
    catch (const std::exception &e) {
        m_Searching = false;
        std::cerr << e.what() << std::endl;
        throw;
    }
}

}