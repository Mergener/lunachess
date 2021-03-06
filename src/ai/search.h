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
#include "aimovegen.h"
#include "basicevaluator.h"
#include "timemanager.h"

#include "../clock.h"
#include "../position.h"

namespace lunachess::ai {

constexpr int MAX_SEARCH_DEPTH = 128;
constexpr int FORCED_MATE_THRESHOLD = 25000000;
constexpr int MATE_SCORE = 30000000;
constexpr int HIGH_BETA = 50000000;

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
    /** How many plies ahead were searched */
    int searchedDepth = 0;

    /** The best move found in the position */
    Move bestMove = MOVE_INVALID;

    /** The score of the position. */
    int bestScore = 0;

    /** The number of visited nodes, including quiescence search nodes. */
    ui64 visitedNodes = 0;

    /** When the search started */
    TimePoint searchStart;

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
     * How much time we've been searching.
     */
    inline const ui64 getSearchTime() const {
        return deltaMs(Clock::now(), searchStart);
    }

    /**
     * How much time we've been searching in the current depth.
     * Read 'currDepthStart' documentation for more precise information.
     */
    inline const ui64 getCurrDepthTime() const {
        return deltaMs(Clock::now(), currDepthStart) + 1;
    }

    /**
     * The number of nodes being searched every second.
     */
    inline const ui64 getNPS() const {
        return static_cast<ui64>(static_cast<double>(visitedNodes) / getCurrDepthTime() * 1000);
    }
};

struct SearchSettings {
    //
    // General settings
    //
    int multiPvCount = 1;

    /**
     * Predicate that should return true only to moves that should be searched in the root node.
     * If moveFilter == nullptr, then the search will not filter out any moves.
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
    std::function<void(SearchResults)> onDepthFinish;
    std::function<void(SearchResults, int pvIdx)> onPvFinish;
};

class AlphaBetaSearcher {
public:
    inline void stop() {
        m_ShouldStop = true;
    }

    SearchResults search(const Position& pos, SearchSettings settings = SearchSettings());

    /**
     * Performs a quiescence search. The quiescence search only searches for 'noisy' moves.
     * For Luna, noisy moves consist of captures and promotions.
     *
     * @param pos The position to search.
     * @param ply The current search ply.
     * @param alpha The alpha (lowerbound) value. The returned score is always greater than or equal to alpha.
     * @param beta The beta (upperbound) value. The returned socre is always less than or equal to beta.
     * @return The score of the position after the quiescence search.
     */
    inline int quiesce(const Position& pos, int ply = 0, int alpha = -HIGH_BETA, int beta = HIGH_BETA) {
        m_Pos = pos;
        return quiesce(ply, alpha, beta);
    }

    inline AlphaBetaSearcher()
        : m_Eval(new BasicEvaluator()), m_PvIt(m_Pv.begin()) {
    }

    /**
     * Constructs a move searcher with an externally created evaluator.
     */
    inline AlphaBetaSearcher(std::shared_ptr<BasicEvaluator> eval)
        : m_Eval(eval), m_PvIt(m_Pv.begin()) {
    }

    inline BasicEvaluator& getEvaluator() { return *m_Eval; }
    inline const BasicEvaluator& getEvaluator() const { return *m_Eval; }

    inline const TranspositionTable& getTT() const { return m_TT; }
    inline TranspositionTable& getTT() { return m_TT; }

private:
    Position m_Pos = Position::getInitialPosition();
    TranspositionTable m_TT;
    SearchResults m_LastResults;
    AIMoveFactory m_MvFactory;
    std::shared_ptr<BasicEvaluator> m_Eval;
    TimeManager m_TimeManager;
    bool m_ShouldStop = false;
    bool m_Searching = false;

    using TPV = std::array<Move, MAX_SEARCH_DEPTH>;
    TPV m_Pv;
    TPV::iterator m_PvIt;

    int alphaBeta(int depth, int ply, int alpha, int beta, bool nullMoveAllowed = true, MoveList* searchMoves = nullptr);

    int quiesce(int ply, int alpha, int beta);

    void pushMoveToPv(TPV::iterator& pvStart, Move move);

    int generateAndOrderMoves(MoveList& ml, int ply, Move pvMove);
    int generateAndOrderMovesQuiesce(MoveList& ml, int ply);
};

}

#endif // LUNA_AI_SEARCH_H