#ifndef LUNA_AI_SEARCH_H
#define LUNA_AI_SEARCH_H

#include <stack>
#include <memory>
#include <functional>
#include <optional>
#include <vector>
#include <atomic>

#include "transpositiontable.h"
#include "evaluator.h"
#include "movecursor.h"
#include "hce/hce.h"
#include "timemanager.h"
#include "searchtrace.h"

#include "../clock.h"
#include "../position.h"

namespace lunachess::ai {

constexpr int MAX_SEARCH_DEPTH = 128;
constexpr int FORCED_MATE_THRESHOLD = 25000000;
constexpr int MATE_SCORE = 30000000;
constexpr int HIGH_BETA = 1000000000;

struct SearchedVariation {
    /**
     * The moves played in this variation.
     * Note that moves are sorted according to the order
     * they are played in the variation.
     */
    std::vector<Move> moves;

    /** The score of the variation. */
    int score = 0;

    /** Determines whether the score was a lowerbound, upperbound or an exact value. */
    TranspositionTable::EntryType type = TranspositionTable::EXACT;
};

struct SearchResults {
    /** The requested depth of this search. */
    int depth = 0;

    /** How many plies ahead were searched at max (excluding quiescence search). */
    int selDepth = 0;

    /** The best move found in the position */
    Move bestMove = MOVE_INVALID;

    /** The score of the position. */
    int bestScore = 0;

    /** The number of visited nodes, including quiescence search nodes. */
    ui64 visitedNodes = 0;

    /** When the search started */
    TimePoint searchStart;

    /** Whether the results of this search were found on cache (TT). */
    bool cached = false;

    /**
     * Following the iterative deepening algorithm, a search is done depth-by-depth,
     * starting at depth 1.
     * 'currDepthStart' marks the timepoint in which we started searching at the current
     * depth.
     */
    TimePoint currDepthStart;

    /**
     * Contains all searched variations and their respective scores.
     * Note that the scores might be lowerbounds/upperbounds.
     */
    std::vector<SearchedVariation> searchedVariations;

    /**
     * The variation in which each player selected the best moves according to Luna.
     */
    inline const SearchedVariation& getPrincipalVariation() const { return searchedVariations[0]; }

    /**
     * How much time we've been searching in milliseconds.
     */
    inline ui64 getSearchTime() const {
        return deltaMs(Clock::now(), searchStart);
    }

    /**
     * How much time we've been searching in the current depth.
     * Read 'currDepthStart' documentation for more precise information.
     */
    inline ui64 getCurrDepthTime() const {
        return deltaMs(Clock::now(), currDepthStart) + 1;
    }

    /**
     * The number of nodes being searched every second.
     */
    inline ui64 getNPS() const {
        return static_cast<ui64>(static_cast<double>(visitedNodes) / getSearchTime() * 1000);
    }

    /**
     * Data structure that contains all paths taken by this search and results found along the way.
     * This tree is only created if "trace" is set to true in search settings. Otherwise, this is set
     * to nullptr.
     */
    std::shared_ptr<SearchTree> traceTree = nullptr;
};

struct SearchSettings {
    //
    // General settings
    //
    int multiPvCount = 1;
    int maxDepth = MAX_SEARCH_DEPTH;

    /**
     * Predicate that must return true only to moves that should be searched in the root node.
     * If moveFilter == nullptr, the search will not filter out any moves.
     */
    std::function<bool(Move)> moveFilter = nullptr;

    //
    // Time control settings
    //
    TimeControl ourTimeControl;
    TimeControl theirTimeControl;

    //
    // Event handlers
    //
    std::function<void(const SearchResults&)> onDepthFinish;
    std::function<void(const SearchResults&, int pvIdx)> onPvFinish;

    bool trace = false;
};

class AlphaBetaSearcher {
public:
    inline void stop() {
        m_ShouldStop = true;
    }

    inline bool searching() const {
        return m_Searching;
    }

    SearchResults search(const Position& pos, SearchSettings settings = SearchSettings());

    inline AlphaBetaSearcher()
        : m_Eval(new HandCraftedEvaluator()) {
    }

    /**
     * Constructs a move searcher with an externally created evaluator.
     */
    inline explicit AlphaBetaSearcher(std::shared_ptr<Evaluator> eval)
        : m_Eval(std::move(eval)) {
    }

    inline AlphaBetaSearcher& operator=(const AlphaBetaSearcher& other) {
        m_Eval = other.m_Eval;
        return *this;
    }

    inline const TranspositionTable& getTT() const { return m_TT; }
    inline TranspositionTable& getTT() { return m_TT; }

    inline Evaluator& getEvaluator() const {
        return *m_Eval;
    }

private:
    TranspositionTable m_TT;
    SearchResults      m_Results;
    MoveOrderingData   m_MvOrderData;
    TimeManager        m_TimeManager;
    SearchTracer       m_Tracer;
    MoveList           m_RootMoves;
    std::shared_ptr<Evaluator> m_Eval;

    bool m_ShouldStop = false;
    bool m_Searching  = false;

    enum SearchFlags {

        NO_SEARCH_FLAGS,
        ROOT      = BIT(0),
        SKIP_NULL = BIT(1),
        ZW        = BIT(2)

    };

    template <bool TRACE, SearchFlags FLAGS = NO_SEARCH_FLAGS>
    int pvs(int depth, int ply, int alpha, int beta, Move moveToSkip = MOVE_INVALID);

    template <bool TRACE>
    int quiesce(int ply, int alpha, int beta);

    /**
     * Checks whether the current search should stop, either if its time
     * is over or it was requested to stop.
     * If it should, stops it by throwing 'SearchInterrupt'.
     */
    void interruptSearchIfNecessary();

    bool isBadCapture(Move move) const;

    template <bool TRACE>
    SearchResults searchInternal(const Position& argPos, SearchSettings settings = SearchSettings());
};

void initializeSearchParameters();
}

#endif // LUNA_AI_SEARCH_H