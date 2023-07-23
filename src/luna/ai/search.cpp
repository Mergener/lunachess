#include "search.h"

#include <algorithm>
#include <thread>

namespace lunachess::ai {

#define TRACE_NEW_TREE(pos, depth) if constexpr (TRACE) { m_Tracer.newTree(pos, depth); }
#define TRACE_PUSH(move)           if constexpr (TRACE) { m_Tracer.push(move); }
#define TRACE_POP()                if constexpr (TRACE) { m_Tracer.pop(); }
#define TRACE_ADD_FLAGS(flags)     if constexpr (TRACE) { m_Tracer.addFlags(flags); }
#define TRACE_SET_SCORE(score)     if constexpr (TRACE) { m_Tracer.setScore(score); }
#define TRACE_SET_STATEVAL(eval)   if constexpr (TRACE) { m_Tracer.setStaticEval(eval); }
#define TRACE_FINISH_TREE(out)     if constexpr (TRACE) { out = m_Tracer.finishTree(); }

/**
 * Pseudo-exception to be thrown when the search must be interrupted.
 */
class SearchInterrupt {
};

constexpr int CHECK_TIME_NODE_INTERVAL = 1024;

static std::array<std::array<int, MoveList::MAX_ELEMS>, MAX_SEARCH_DEPTH * 2> s_LMRReductions;

void initializeSearchParameters() {
    for (int depth = 0; depth < s_LMRReductions.size(); ++depth) {
        auto& lmrPerMove = s_LMRReductions[depth];
        for (int m = 0; m < lmrPerMove.size(); ++m) {
            lmrPerMove[m] = static_cast<int>(1.25 + std::log(depth) * std::log(m) * 100 / 267);
        }
    }
}

static int getLMRReduction(int depth, int moveIndex) {
    return s_LMRReductions[depth][moveIndex];
}

void AlphaBetaSearcher::interruptSearchIfNecessary() {
    if (m_Results.visitedNodes % CHECK_TIME_NODE_INTERVAL == 0 &&
        (m_TimeManager.timeIsUp()
         || m_ShouldStop)) {
        throw SearchInterrupt();
    }
}

template <bool TRACE>
int AlphaBetaSearcher::quiesce(int ply, int alpha, int beta) {
    const Position& pos = m_Eval->getPosition();
    m_Results.visitedNodes++;

    interruptSearchIfNecessary();

    int standPat = m_Eval->evaluate();
    TRACE_SET_STATEVAL(standPat);

    if (standPat >= beta) {
        // Fail high
        TRACE_ADD_FLAGS(STF_BETA_CUTOFF);
        TRACE_SET_SCORE(beta);
        return beta;
    }

    if (standPat > alpha) {
        // Before we search, if the standPat is higher than alpha then it means
        // that not performing a noisy move is best (so far).
        alpha = standPat;
    }

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
        TRACE_SET_SCORE(alpha);
        return alpha;
    }
    // #----------------------------------------

    MoveCursor<true> moveCursor;
    Move move;
    while ((move = moveCursor.next(pos, m_MvOrderData, ply))) {
        if (move.getType() == MT_SIMPLE_CAPTURE &&
            !staticanalysis::hasGoodSEE(pos, move)) {
            // The result of the exchange series should result in
            // material loss after this capture, prune it.
            continue;
        }

        TRACE_PUSH(move);
        m_Eval->makeMove(move);

        int score = -quiesce<TRACE>(ply + 1, -beta, -alpha);

        m_Eval->undoMove();
        TRACE_POP();

        if (score >= beta) {
            // Fail high
            TRACE_ADD_FLAGS(STF_BETA_CUTOFF);
            TRACE_SET_SCORE(beta);
            return beta;
        }

        if (score > alpha) {
            alpha = score;
        }
    }

    TRACE_SET_SCORE(alpha);
    return alpha;
}

bool AlphaBetaSearcher::isBadCapture(Move move) const {
    return !staticanalysis::hasGoodSEE(m_Eval->getPosition(), move);
}

template <bool TRACE, AlphaBetaSearcher::SearchFlags FLAGS>
int AlphaBetaSearcher::pvs(int depth, int ply,
                           int alpha, int beta,
                           MoveList *searchMoves) {
    const Position& pos = m_Eval->getPosition();

    if (!BIT_INTERSECTS(FLAGS, ROOT) && pos.isDraw()) {
        // Position is a draw, return draw score.
        TRACE_SET_SCORE(m_Eval->getDrawScore());
        TRACE_SET_STATEVAL(m_Eval->evaluate());
        return m_Eval->getDrawScore();
    }

    interruptSearchIfNecessary();

    // Setup some important variables
    int staticEval = 0; // Used for some pruning/reduction techniques
    const int originalDepth = depth;
    const int originalAlpha = alpha;
    Move hashMove = MOVE_INVALID; // Move extracted from TT

    // Probe the transposition table
    ui64 posKey = pos.getZobrist();
    TranspositionTable::Entry ttEntry = {};
    ttEntry.zobristKey = posKey;
    ttEntry.move = MOVE_INVALID;

    bool foundInTT = m_TT.probe(posKey, ttEntry);
    if (foundInTT) {
        staticEval = ttEntry.staticEval;
        TRACE_SET_STATEVAL(ttEntry.staticEval);

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
                        m_Results.visitedNodes++;

                        TRACE_SET_SCORE(ttEntry.score);
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
                    m_Results.visitedNodes++;

                    TRACE_SET_SCORE(ttEntry.score);
                    return ttEntry.score;
                }
            }
        }
    }
    // #----------------------------------------

    if (depth <= 0) {
        return quiesce<TRACE>(ply, alpha, beta);
    }
    m_Results.visitedNodes++;

    bool isCheck = pos.isCheck();
    if (!isCheck) {
        // Only decrease depth if we're not currently in check
        depth--;

        if (!foundInTT) {
            // No TT entry found and we're not in check, we need to compute the static eval here.
            staticEval = m_Eval->evaluate();
            TRACE_SET_STATEVAL(staticEval);
        }
    }

    int drawScore = m_Eval->getDrawScore();

    if (BIT_INTERSECTS(FLAGS, ZW) && !isCheck) {
        int pieceCount = pos.getBitboard(Piece(pos.getColorToMove(), PT_NONE)).count();

        // #----------------------------------------
        // # NULL MOVE PRUNING
        // #----------------------------------------
        // Prune if making a null move fails high
        constexpr int NULL_SEARCH_DEPTH_RED = 2;
        constexpr int NULL_SEARCH_MIN_DEPTH = NULL_SEARCH_DEPTH_RED + 1;
        constexpr int NULL_MOVE_MIN_PIECES = 4;

        if (!BIT_INTERSECTS(FLAGS, SKIP_NULL) &&
            depth >= NULL_SEARCH_MIN_DEPTH &&
            pieceCount > NULL_MOVE_MIN_PIECES) {

            // Null move pruning allowed
            TRACE_PUSH(MOVE_INVALID);
            m_Eval->makeNullMove();

            int score = -pvs<TRACE, SKIP_NULL>(depth - NULL_SEARCH_DEPTH_RED, ply + 1, -beta, -beta + 1);
            if (score >= beta) {
                //depth -= NULL_SEARCH_DEPTH_RED;
                TRACE_POP();
                m_Eval->undoNullMove();

                TRACE_SET_SCORE(beta);
                return beta; // Prune
            }

            m_Eval->undoNullMove();
            TRACE_POP();
        }
        // #----------------------------------------
    }

    // Save last move. Used in countermove heuristic.
    Move lastMove = pos.getLastMove();

    // Finally, do the search
    bool shouldSearchPV = true;

    MoveCursor moveCursor;
    Move bestMove = moveCursor.next(pos, m_MvOrderData, ply, hashMove);
    int searchedMoves = 0;
    for (Move move = bestMove; move != MOVE_INVALID; move = moveCursor.next(pos, m_MvOrderData, ply)) {
        if (BIT_INTERSECTS(FLAGS, ROOT) &&
            searchMoves != nullptr &&
            !searchMoves->contains(move)) {
            // Don't search for unrequested moves
            continue;
        }

        searchedMoves++;
        TRACE_PUSH(move);
        m_Eval->makeMove(move);

        // The highest depth we're going to search during this iteration.
        // Used in PV nodes or researches after fail highs.
        int fullIterationDepth = depth;

        // #----------------------------------------
        // # FUTILITY PRUNING
        // #----------------------------------------
        // Prune frontier/pre-frontier nodes with no chance of improving evaluation.
        constexpr int FUTILITY_MARGIN = 2500;
        if (!BIT_INTERSECTS(FLAGS, ROOT) && !pos.isCheck() && move.is<MTM_QUIET>()) {
            if (fullIterationDepth == 1 && (staticEval + FUTILITY_MARGIN) < alpha) {
                // Prune
                m_Eval->undoMove();
                TRACE_POP();
                continue;
            }
            if (fullIterationDepth == 2 && (staticEval + FUTILITY_MARGIN * 2) < alpha) {
                // Prune
                m_Eval->undoMove();
                TRACE_POP();
                continue;
            }
        }
        // #----------------------------------------

        int iterationDepth = fullIterationDepth;

        // #----------------------------------------
        // # LATE MOVE REDUCTIONS
        // #----------------------------------------
        if (!shouldSearchPV) {
            if (depth >= 2 &&
                !pos.isCheck() &&
                searchedMoves >= 2 &&
                (move.is<MTM_QUIET>() || (isBadCapture(move)))) {
                int reduction = getLMRReduction(iterationDepth, searchedMoves);

                iterationDepth -= std::max(0, reduction);
            }
        }
        // #----------------------------------------

        int score;
        if (shouldSearchPV) {
            // Perform PVS. First move of the list is always PVS.
            score = -pvs<TRACE>(iterationDepth, ply + 1, -beta, -alpha);
        }
        else {
            // Perform a ZWS. Searches after the first move are performed
            // with a null window. If the search fails high, do a re-search
            // with the full window.
            score = -pvs<TRACE, ZW>(iterationDepth, ply + 1, -alpha - 1, -alpha);
            if (score > alpha) {
                iterationDepth = fullIterationDepth;
                score = -pvs<TRACE>(iterationDepth, ply + 1, -beta, -alpha);
            }
        }

        if (score >= FORCED_MATE_THRESHOLD) {
            score--;
        }
        else if (score <= -FORCED_MATE_THRESHOLD) {
            score++;
        }

        m_Eval->undoMove();
        TRACE_POP();

        if (score >= beta) {
            // Beta cutoff, fail high
            alpha = beta;

            bestMove = move;

            if (move.is<MTM_QUIET>()) {
                m_MvOrderData.storeHistory(move, iterationDepth);
                m_MvOrderData.storeKillerMove(move, ply);
                m_MvOrderData.storeCounterMove(lastMove, move);
            }
            break;
        }
        if (score > alpha) {
            alpha = score;
            bestMove = move;
        }
        shouldSearchPV = false;
    }

    if (searchedMoves == 0) {
        // Either stalemate or checkmate
        if (isCheck) {
            TRACE_SET_SCORE(-MATE_SCORE);
            return -MATE_SCORE;
        }
        TRACE_SET_SCORE(drawScore);
        return drawScore;
    }

    // Store search data in transposition table
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

    ttEntry.depth      = originalDepth;
    ttEntry.move       = bestMove;
    ttEntry.score      = alpha;
    ttEntry.zobristKey = posKey;
    ttEntry.staticEval = staticEval;
    m_TT.maybeAdd(ttEntry);

    TRACE_SET_SCORE(alpha);

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

template <bool TRACE>
SearchResults AlphaBetaSearcher::searchInternal(const Position &argPos, SearchSettings settings) {
    while (m_Searching); // Wait current search.

    m_Results = {};

    try {
        // Reset everything
        m_Searching = true;
        m_ShouldStop = false;
        m_MvOrderData.resetAll();

        // Setup variables
        m_Eval->setPosition(argPos);
        const Position& pos = m_Eval->getPosition();
        int drawScore = m_Eval->getDrawScore();
        int maxDepth = std::min(MAX_SEARCH_DEPTH, settings.maxDepth);

        // Try to generate moves in the position before we search
        MoveList moves;
        movegen::generate(pos, moves);
        if (moves.size() == 0) {
            int score = pos.isCheck()
                        ? -MATE_SCORE // Checkmate
                        : drawScore;  // Stalemate

            m_Results.bestScore = score;
            m_Results.bestMove = MOVE_INVALID;
            m_Searching = false;

            return std::move(m_Results);
        }

        // Filter out undesired moves (usually as specified by UCI 'searchmoves')
        filterMoves(moves, settings.moveFilter);

        // Setup results object
        m_Results.visitedNodes = 1;
        m_Results.searchStart = Clock::now();

        // Last search results could have been filled with a previous search
        m_Results.searchedVariations.clear();

        m_Results.bestMove = moves[0];
        m_Results.searchedVariations.resize(settings.multiPvCount);

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

                TRACE_NEW_TREE(pos, depth);

                // Prepare results object for this search
                m_Results.currDepthStart = Clock::now();

                // Perform the search
                try {
                    TranspositionTable::Entry ttEntry;

                    constexpr int ASPIRATION_WINDOWS_MIN_DEPTH = 3;
                    constexpr int MAX_ASPIRATION_ITERATIONS = 0;

                    int alpha;
                    double alphaDelta = 1;
                    int beta;
                    double betaDelta = 1;

                    int score;
                    int lastScore = m_Results.bestScore;
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

                        score = pvs<TRACE, ROOT>(depth, 0, alpha, beta, &moves);
                        if (score <= alpha) {
                            // Fail low, widen lower bound.
                            alpha -= static_cast<int>(alphaDelta * 1000);
                            alphaDelta += std::pow(alphaDelta + 0.6, alphaDelta + 0.3);
                        }
                        else if (score >= beta) {
                            // Fail high, increase lower bound
                            beta += static_cast<int>(betaDelta * 1000);
                            betaDelta += std::pow(betaDelta + 0.6, betaDelta + 0.3);
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
                        m_Results.bestScore = score;
                        if (ttEntry.move != MOVE_INVALID) {
                            m_Results.bestMove = ttEntry.move;
                        }
                        m_Results.searchedDepth = depth;
                    }
                    moves.remove(ttEntry.move);


                    // We finished the search on this variation at this depth.
                    // Now, properly fill the searched variation object for this pv
                    // in the results object.
                    auto &pv = m_Results.searchedVariations[multipv];
                    pv.score = ttEntry.score;
                    pv.type = TranspositionTable::EXACT;

                    // Check for moves on the PV in transposition table
                    pv.moves.clear();
                    while (m_TT.probe(pos, ttEntry) && ttEntry.move != MOVE_INVALID) {
                        TRACE_PUSH(ttEntry.move);
                        TRACE_ADD_FLAGS(STF_PV);
                        pv.moves.push_back(ttEntry.move);
                        m_Eval->makeMove(ttEntry.move);

                        if (pos.isRepetitionDraw()) {
                            break;
                        }
                    }

                    // Undo all moves
                    for (int i = 0; i < pv.moves.size(); ++i) {
                        TRACE_POP();
                        m_Eval->undoMove();
                    }

                    // When using multi PVs, we need to clear the entry of the initial
                    // search position.
                    if (settings.multiPvCount > 1) {
                        m_TT.remove(pos);
                    }

                    // Notify handler
                    if (settings.onPvFinish != nullptr) {
                        settings.onPvFinish(m_Results, multipv);
                    }
                }
                catch (const SearchInterrupt &) {
                    // Time over
                    break;
                }

                TRACE_FINISH_TREE(m_Results.traceTree);
            }

            moves.clear();
            movegen::generate(pos, moves);
            filterMoves(moves, settings.moveFilter);

            m_TimeManager.onNewDepth(m_Results);
            if (settings.onDepthFinish != nullptr) {
                settings.onDepthFinish(m_Results);
            }
        }

        m_Searching = false;

        return m_Results;
    }
    catch (const std::exception &e) {
        m_Searching = false;
        std::cerr << e.what() << std::endl;
        throw;
    }
}

SearchResults AlphaBetaSearcher::search(const Position &argPos, SearchSettings settings) {
    if (settings.trace) {
        return searchInternal<true>(argPos, settings);
    }
    return searchInternal<false>(argPos, settings);
}

}