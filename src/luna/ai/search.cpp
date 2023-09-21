#include "search.h"

#include <algorithm>
#include <thread>

namespace lunachess::ai {

#define TRACE_NEW_TREE(pos, depth)    if constexpr (TRACE) { m_Tracer.newTree(pos, depth); }
#define TRACE_PUSH(move)              if constexpr (TRACE) { m_Tracer.push(move); }
#define TRACE_POP()                   if constexpr (TRACE) { m_Tracer.pop(); }
#define TRACE_DEPTH(depth)            if constexpr (TRACE) { m_Tracer.setRequestedDepth(depth); }
#define TRACE_ADD_FLAGS(flags)        if constexpr (TRACE) { m_Tracer.addFlags(flags); }
#define TRACE_SET_SCORES(score, a, b) if constexpr (TRACE) { m_Tracer.setScores(score, a, b); }
#define TRACE_SET_STATEVAL(eval)      if constexpr (TRACE) { m_Tracer.setStaticEval(eval); }
#define TRACE_UPDATE_BEST_MOVE(move)  if constexpr (TRACE) { m_Tracer.updateBestMove(move); }
#define TRACE_FINISH_TREE(out)        if constexpr (TRACE) { out = m_Tracer.finishTree(); }

/**
 * Pseudo-exception to be thrown when the search must be interrupted.
 */
class SearchInterrupt {
};

constexpr int CHECK_TIME_NODE_INTERVAL = 2048;
static_assert((CHECK_TIME_NODE_INTERVAL & (CHECK_TIME_NODE_INTERVAL - 1)) == 0,
        "CHECK_TIME_NODE_INTERVAL must be a power of 2");

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
    TRACE_DEPTH(0);

    const Position& pos = m_Eval->getPosition();
    m_Results.visitedNodes++;

    interruptSearchIfNecessary();

    int standPat = m_Eval->evaluate();
    TRACE_SET_STATEVAL(standPat);

    if (standPat >= beta) {
        // Fail high
        TRACE_ADD_FLAGS(STF_BETA_CUTOFF);
        TRACE_SET_SCORES(beta, alpha, beta);
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
    int bigDelta = 16000;

    // Check whether we have pawns that can be promoted
    Bitboard promoters = pos.getColorToMove() == CL_WHITE
                         ? bbs::getRankBitboard(RANK_7)
                         : bbs::getRankBitboard(RANK_2);
    promoters &= pos.getBitboard(Piece(pos.getColorToMove(), PT_PAWN));
    if (promoters > 0) {
        bigDelta += 16000;
    }

    if (standPat < alpha - bigDelta) {
        // No material delta could improve our position enough, we can
        // perform some pruning.
        TRACE_SET_SCORES(alpha, alpha, beta);
        return alpha;
    }
    // #----------------------------------------

    int bestItScore = -HIGH_BETA;
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

        if (score > bestItScore) {
            bestItScore = score;
            TRACE_UPDATE_BEST_MOVE(move);

            if (score >= beta) {
                // Fail high
                TRACE_ADD_FLAGS(STF_BETA_CUTOFF);
                TRACE_SET_SCORES(beta, alpha, beta);
                return beta;
            }

            if (score > alpha) {
                alpha = score;
            }
        }
    }

    TRACE_SET_SCORES(alpha, alpha, beta);
    return alpha;
}

bool AlphaBetaSearcher::isBadCapture(Move move) const {
    return !staticanalysis::hasGoodSEE(m_Eval->getPosition(), move);
}

static i32 convertSearchScoreToTT(i32 searchScore, i32 ply) {
    if (searchScore >= FORCED_MATE_THRESHOLD) {
        return searchScore + ply;
    }
    if (searchScore <= -FORCED_MATE_THRESHOLD) {
        return searchScore - ply;
    }
    return searchScore;
}

static i32 convertTTScoreToSearch(i32 ttScore, i32 ply) {
    if (ttScore >= FORCED_MATE_THRESHOLD) {
        return ttScore - ply;
    }
    if (ttScore <= -FORCED_MATE_THRESHOLD) {
        return ttScore + ply;
    }
    return ttScore;
}

template <bool TRACE, AlphaBetaSearcher::SearchFlags FLAGS>
int AlphaBetaSearcher::pvs(int depth, int ply,
                           int alpha, int beta,
                           Move moveToSkip) {
    constexpr bool IS_ROOT = BIT_INTERSECTS(FLAGS, ROOT);
    constexpr bool IS_ZW   = BIT_INTERSECTS(FLAGS, ZW);
    constexpr bool DO_NMP  = !BIT_INTERSECTS(FLAGS, SKIP_NULL);

    m_Results.selDepth = std::max(m_Results.selDepth, ply);
    TRACE_DEPTH(depth);
    const Position& pos = m_Eval->getPosition();

    // Check for draws.
    // When in root, we always assume the user wants to "force" Luna to analyze
    // even a drawn position, so we don't care if its drawn.
    if (!IS_ROOT &&
        (pos.isRepetitionDraw(2) ||
        pos.is50MoveRuleDraw() || pos.isInsufficientMaterialDraw())) {
        // Position is a draw, return draw score.
        TRACE_SET_SCORES(m_Eval->getDrawScore(), alpha, beta);
        TRACE_SET_STATEVAL(m_Eval->evaluate());
        return m_Eval->getDrawScore();
    }

    interruptSearchIfNecessary();

    // Setup some important variables.
    int staticEval          = 0; // Used for some pruning/reduction techniques
    const int originalDepth = depth;
    const int originalAlpha = alpha;
    Move hashMove           = MOVE_INVALID; // Move extracted from TT
    ui64 posKey             = pos.getZobrist();

    // Probe the transposition table.
    TranspositionTable::Entry ttEntry = {};
    ttEntry.zobristKey = posKey;
    ttEntry.move       = MOVE_INVALID;

    bool foundInTT = m_TT.probe(posKey, ttEntry);
    if (foundInTT) {
        // We found the current position on the TT.
        ttEntry.score = convertTTScoreToSearch(ttEntry.score, ply);
        staticEval = ttEntry.staticEval;
        TRACE_SET_STATEVAL(ttEntry.staticEval);

        // We need to respect the restriction of root moves list in case we're in the
        // root of the search.
        if (!IS_ROOT || m_RootMoves.contains(ttEntry.move)) {
            hashMove = ttEntry.move;
            LUNA_ASSERT(pos.isMovePseudoLegal(hashMove),
                        "Hash move " << hashMove << " is not pseudo legal.");
            LUNA_ASSERT(pos.isMoveLegal(hashMove),
                        "Hash move " << hashMove << " is not legal.");

            if (!IS_ROOT && ttEntry.depth >= depth) {
                // We always accept higher or equal depth TT scores unless they're
                // mate scores. This mate score condition is a temporary workaround
                // for situations in which Luna repeats moves and ends up drawing mating positions
                // when she finds mate scores in TT entries.
                if (ttEntry.type == TranspositionTable::EXACT) {
                    m_Results.visitedNodes++;

                    TRACE_UPDATE_BEST_MOVE(ttEntry.move);
                    TRACE_SET_SCORES(ttEntry.score, alpha, beta);

                    if constexpr (IS_ROOT) {
                        // Update results best move.
                        m_Results.bestScore = ttEntry.score;
                        m_Results.bestMove  = ttEntry.move;
                        m_Results.cached    = true;
                    }

                    return ttEntry.score;
                }
                else if (ttEntry.type == TranspositionTable::LOWERBOUND) {
                    alpha = std::max(alpha, ttEntry.score);
                }
                else if (ttEntry.type == TranspositionTable::UPPERBOUND) {
                    beta = std::min(beta, ttEntry.score);
                }

                if (alpha >= beta) {
                    m_Results.visitedNodes++;

                    TRACE_UPDATE_BEST_MOVE(ttEntry.move);
                    TRACE_SET_SCORES(ttEntry.score, alpha, beta);
                    return ttEntry.score;
                }
            }
        }
    }

    if (depth <= 0) {
        return quiesce<TRACE>(ply, alpha, beta);
    }

    m_Results.visitedNodes++;

    bool isCheck = pos.isCheck();
    if (!isCheck && !foundInTT) {
        // No TT entry found and we're not in check, we need to compute the static eval here.
        staticEval = m_Eval->evaluate();
        TRACE_SET_STATEVAL(staticEval);
    }

    // #----------------------------------------
    // # INTERNAL ITERATIVE DEEPENING
    // #----------------------------------------
    if (!foundInTT && depth > 1) {
        depth--;
    }
    // #----------------------------------------

    // #----------------------------------------
    // # REVERSE FUTILITY PRUNING
    // # (A.K.A STATIC NULL MOVE PRUNING)
    // #----------------------------------------
    // Prune if we have a position that is too good.
    int rfpMargin = 500 + 700 * depth;
    if (IS_ZW      &&
        !isCheck   &&
        depth <= 7 &&
        alpha < FORCED_MATE_THRESHOLD &&
        staticEval - rfpMargin > beta) {
        TRACE_SET_SCORES(staticEval - rfpMargin, alpha, beta);
        return staticEval - rfpMargin;
    }
    // #----------------------------------------

    // #----------------------------------------
    // # RAZORING
    // #----------------------------------------
    // If we're in a line that is so much worse than our expected lowerbound,
    // the chances of us improving our positions get increasingly lower the further we go.
    // Reduce the depth.
    int margin = 1200 + 800 * depth;
    int evalPlusMargin = staticEval + margin;
    if (IS_ZW    &&
        !isCheck &&
        (evalPlusMargin < alpha) &&
        depth > 5) {
        int quiesceScore = quiesce<false>(ply, evalPlusMargin - 1, alpha);
        if (quiesceScore < evalPlusMargin) {
            TRACE_SET_SCORES(quiesceScore, alpha, beta);
            depth = (depth * 2) / 3;
        }
    }
    // #----------------------------------------

    int drawScore = m_Eval->getDrawScore();

    if (IS_ZW && !isCheck) {
        int pieceCount = pos.getBitboard(Piece(pos.getColorToMove(), PT_NONE)).count();

        // #----------------------------------------
        // # NULL MOVE PRUNING
        // #----------------------------------------
        // Prune if making a null move fails high
        constexpr int NULL_SEARCH_DEPTH_RED = 2;
        constexpr int NULL_SEARCH_MIN_DEPTH = NULL_SEARCH_DEPTH_RED + 1;
        constexpr int NULL_MOVE_MIN_PIECES  = 4;

        if (DO_NMP && staticEval >= beta   &&
            depth >= NULL_SEARCH_MIN_DEPTH &&
            pieceCount > NULL_MOVE_MIN_PIECES) {

            // Null move pruning allowed
            TRACE_PUSH(MOVE_INVALID);
            m_Eval->makeNullMove();

            int score = -pvs<TRACE, SKIP_NULL>(depth - NULL_SEARCH_DEPTH_RED - 1, ply + 1, -beta, -beta + 1);
            if (score >= beta) {
                TRACE_POP();
                m_Eval->undoNullMove();

                TRACE_SET_SCORES(beta, alpha, beta);
                TRACE_ADD_FLAGS(STF_BETA_CUTOFF);
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
    int bestItScore     = -HIGH_BETA;
    int searchedMoves   = 0;
    int searchedDepth   = 0;
    bool hasLegalMoves  = false;

    MoveCursor moveCursor;
    Move bestMove = moveCursor.next(pos, m_MvOrderData, ply, hashMove);

    for (Move move = bestMove;
         move != MOVE_INVALID;
         move = moveCursor.next(pos, m_MvOrderData, ply, depth)) {
        hasLegalMoves = true;
        if (IS_ROOT &&
            !m_RootMoves.contains(move)) {
            // Don't search for unrequested moves
            continue;
        }

        if (move == moveToSkip) {
            continue;
        }

        // The highest depth we're going to search during this iteration.
        // Used in PV nodes or researches after fail highs.
        int fullIterationDepth = depth;

        // #----------------------------------------
        // # SINGULAR EXTENSIONS
        // #----------------------------------------
        // Try to see if we have a singular move. In other words, check if
        // only one move works in the current position.
        // If so, extend it.
        bool extendedSingular = false;
        if (!IS_ROOT   &&
            depth >= 8 &&
            foundInTT  &&
            moveToSkip == MOVE_INVALID &&
            ttEntry.depth >= depth - 3 && // Limits too many extensions
            ttEntry.score < FORCED_MATE_THRESHOLD &&
            (ttEntry.type == TranspositionTable::EXACT || ttEntry.type == TranspositionTable::LOWERBOUND) &&
            move == hashMove) {
            int seBeta = std::min(beta, ttEntry.score);

            int score = pvs<TRACE, ZW>((depth - 1) / 2, ply + 1, seBeta - 1, seBeta, hashMove);
            if (score < seBeta) {
                // We have a singular move. Search it with extended depth
                extendedSingular = true;
                fullIterationDepth++;
            }
            else if (score >= beta) {
                return score;
            }
        }
        // #----------------------------------------
        searchedMoves++;

        // #----------------------------------------
        // # SEE PRUNING
        // #----------------------------------------
        // Prune moves with bad static exchange evaluation.
        if (IS_ZW      &&
            depth <= 3 &&
            !isCheck   &&
            !staticanalysis::hasGoodSEE(pos, move, -depth + 1)) {
            continue;
        }
        // #----------------------------------------

        TRACE_PUSH(move);
        m_Eval->makeMove(move);
        m_TT.prefetch(pos.getZobrist());

        // #----------------------------------------
        // # CHECK EXTENSIONS
        // #----------------------------------------
        // Extend moves that give check.
        bool moveGivesCheck = pos.isCheck();
        if (moveGivesCheck && !extendedSingular) {
            fullIterationDepth++;
        }
        // #----------------------------------------

        // #----------------------------------------
        // # FUTILITY PRUNING
        // #----------------------------------------
        // Prune frontier/pre-frontier nodes with no chance of improving evaluation.
        constexpr int FUTILITY_MARGIN = 2500;
        if (!IS_ROOT                &&
            !moveGivesCheck         &&
            move.is<MTM_QUIET>()    &&
            fullIterationDepth <= 2 &&
            (staticEval + FUTILITY_MARGIN * fullIterationDepth) < alpha) {
            // Prune
            m_Eval->undoMove();
            TRACE_POP();
            continue;
        }
        // #----------------------------------------

        int iterationDepth = fullIterationDepth;

        // #----------------------------------------
        // # LATE MOVE REDUCTIONS
        // #----------------------------------------
        // Assume that moves towards the end of our search
        // are probably worse than previously searched moves and
        // reduce their search depth.
        if (!shouldSearchPV) {
            if (depth >= 2         &&
                !moveGivesCheck    &&
                searchedMoves >= 2 &&
                (move.is<MTM_QUIET>() || (isBadCapture(move)))) {
                int reduction   = getLMRReduction(iterationDepth, searchedMoves);
                iterationDepth -= std::max(0, reduction);
            }
        }
        // #----------------------------------------

        int score;
        if (shouldSearchPV) {
            // Perform PVS. First move of the list is always PVS.
            score = -pvs<TRACE>(iterationDepth - 1, ply + 1, -beta, -alpha);
        }
        else {
            // Perform a ZWS. Searches after the first move are performed
            // with a null window. If the search fails high, do a re-search
            // with the full window.
            score = -pvs<TRACE, ZW>(iterationDepth - 1, ply + 1, -alpha - 1, -alpha);
            if (score > alpha) {
                iterationDepth = fullIterationDepth;
                score = -pvs<TRACE>(iterationDepth - 1, ply + 1, -beta, -alpha);
            }
        }

        m_Eval->undoMove();
        TRACE_POP();

        if constexpr (IS_ROOT) {
            // Update results best move.
            if (score > alpha) {
                m_Results.bestScore = score;
                m_Results.bestMove  = move;
                m_Results.cached    = false;
            }
        }

        if (score > bestItScore) {
            // We found a new best move for this iteration.
            bestItScore   = score;
            bestMove      = move;
            searchedDepth = iterationDepth;
            TRACE_UPDATE_BEST_MOVE(move);

            if (score >= beta) {
                // Beta cutoff, fail high
                TRACE_ADD_FLAGS(STF_BETA_CUTOFF);

                alpha    = beta;
                bestMove = move;

                if (bestMove.is<MTM_QUIET>()) {
                    m_MvOrderData.storeHistory(bestMove, searchedDepth);
                    m_MvOrderData.storeKillerMove(bestMove, ply);
                    m_MvOrderData.storeCounterMove(lastMove, bestMove);
                }
                break;
            }
            if (score > alpha) {
                alpha = score;
            }
        }
        // Set to false. This should only be true for the first searched
        // move.
        shouldSearchPV = false;
    }

    if (!hasLegalMoves) {
        // Either stalemate or checkmate
        if (isCheck) {
            TRACE_SET_SCORES(-MATE_SCORE, alpha, beta);
            return -MATE_SCORE + ply;
        }
        TRACE_SET_SCORES(drawScore, alpha, beta);
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
    ttEntry.zobristKey = posKey;
    ttEntry.staticEval = staticEval;

    // Mate scores are trickier to handle when saving to TT
    ttEntry.score = convertSearchScoreToTT(alpha, ply);
    m_TT.maybeAdd(ttEntry);

    TRACE_SET_SCORES(alpha, alpha, beta);

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
        m_TT.newGeneration();
        m_Searching  = true;
        m_ShouldStop = false;
        m_MvOrderData.resetAll();

        // Setup variables
        m_Eval->setPosition(argPos);

        const Position& pos = m_Eval->getPosition();
        int drawScore       = m_Eval->getDrawScore();
        int maxDepth        = std::min(MAX_SEARCH_DEPTH, settings.maxDepth);

        // Try to generate moves in the position before we search
        m_RootMoves.clear();
        movegen::generate(pos, m_RootMoves);

        // Filter out undesired moves (usually as specified by UCI 'searchmoves')
        filterMoves(m_RootMoves, settings.moveFilter);

        // Setup results object
        m_Results.visitedNodes = 1;
        m_Results.searchStart  = Clock::now();
        m_Results.selDepth     = 0;
        m_Results.bestMove     = m_RootMoves[0];
        m_Results.searchedVariations.clear();
        m_Results.searchedVariations.resize(settings.multiPvCount);

        // Check for stalemate/checkmate position
        if (m_RootMoves.size() == 0) {
            int score = pos.isCheck()
                        ? -MATE_SCORE // Checkmate
                        : drawScore;  // Stalemate

            m_Results.bestScore = score;
            m_Results.bestMove  = MOVE_INVALID;
            m_Searching         = false;

            return std::move(m_Results);
        }

        // Notify the time manager that we're starting a search
        m_TimeManager.start(settings.ourTimeControl);

        // Perform iterative deepening, starting at depth 1
        for (int depth = 1; depth <= maxDepth; depth++) {
            if (m_TimeManager.timeIsUp() || m_ShouldStop) {
                break;
            }

            if (settings.ourTimeControl.mode == TC_TOURNAMENT &&
                m_RootMoves.size() == 1 &&
                depth > 1) {
                // After scoring the position, stop searching if we only have one legal move.
                break;
            }

            // Caller might be asking for a multi-pv search. Search the number of pvs requested
            // or until all moves were searched
            for (int multipv = 0; multipv < settings.multiPvCount && m_RootMoves.size() > 0; ++multipv) {
                if (m_TimeManager.timeIsUp() || m_ShouldStop) {
                    break;
                }

                // Prepare results object for this search
                m_Results.currDepthStart = Clock::now();

                // Perform the search
                try {
                    TranspositionTable::Entry ttEntry;

                    constexpr int ASPIRATION_WINDOWS_MIN_DEPTH = 3;
                    constexpr int MAX_ASPIRATION_ITERATIONS = 3;

                    int alpha;
                    int beta;

                    int score;
                    int lastScore = m_Results.bestScore;
                    if (depth < ASPIRATION_WINDOWS_MIN_DEPTH) {
                        // By default, perform a full window search
                        alpha = -HIGH_BETA;
                        beta  =  HIGH_BETA;
                    }
                    else {
                        // From depth ASPIRATION_WINDOWS_MIN_DEPTH onwards, try to use aspiration windows based
                        // on the last search.
                        alpha = lastScore - 500;
                        beta  = lastScore + 500;
                    }

                    for (int aspirationIt = 0; aspirationIt <= MAX_ASPIRATION_ITERATIONS; ++aspirationIt) {
                        if (aspirationIt == MAX_ASPIRATION_ITERATIONS) {
                            // We tried many aspiration windows that didn't work, let's go with
                            // the full window.
                            alpha = -HIGH_BETA;
                            beta  =  HIGH_BETA;
                        }

                        TRACE_NEW_TREE(pos, depth);

                        score = pvs<TRACE, ROOT>(depth, 0, alpha, beta);
                        if (score <= alpha) {
                            // Fail low, widen lower bound.
                            TRACE_FINISH_TREE(m_Results.traceTree);
                            alpha -= 500;
                        }
                        else if (score >= beta) {
                            // Fail high, increase lower bound
                            TRACE_FINISH_TREE(m_Results.traceTree);
                            beta += 500;
                        }
                        else {
                            break;
                        }
                    }

                    m_TT.probe(pos, ttEntry);
                    m_Results.depth = depth;

                    m_RootMoves.remove(ttEntry.move);


                    // We finished the search on this variation at this depth.
                    // Now, properly fill the searched variation object for this pv
                    // in the results object.
                    auto &pv = m_Results.searchedVariations[multipv];
                    pv.score = ttEntry.score;
                    pv.type  = TranspositionTable::EXACT;

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

            m_RootMoves.clear();
            movegen::generate(pos, m_RootMoves);
            filterMoves(m_RootMoves, settings.moveFilter);

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