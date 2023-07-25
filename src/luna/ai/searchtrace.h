#ifndef LUNA_SEARCHTRACE_H
#define LUNA_SEARCHTRACE_H

#include <memory>
#include <vector>
#include <ostream>

#include <nlohmann/json.hpp>

#include "../position.h"

namespace lunachess::ai {

constexpr int MAX_CHILDREN_PER_NODE = 256;

enum SearchTreeFlags: ui8 {

    STF_NONE        = 0,
    STF_BETA_CUTOFF = BIT(0),
    STF_PV       = BIT(1)

};

class SearchTree {
    friend class SearchTracer;
    friend std::ostream& operator<<(std::ostream& stream, const SearchTree& tree);

public:
    inline explicit SearchTree(Position rootPos, int expectedNodeCount = 24000)
        : m_RootPos(std::move(rootPos)), m_Nodes(expectedNodeCount) {}

    inline const Position& getRootPosition() {
        return m_RootPos;
    }

    inline int getNodeCount() const {
        return m_Nodes.size();
    }

private:
    struct Node {

        Move lastMove         = MOVE_INVALID;
        Move bestMove         = MOVE_INVALID;
        ui32 parentIndex      = 0;
        i32 staticEval        = 0;
        i32 score             = 0;
        i32 alpha             = alpha;
        i32 beta              = beta;
        ui32 childIndexes[MAX_CHILDREN_PER_NODE] = {}; // zero-initialized
        ui8 nChildren         = 0;
        ui8 requestedDepth    = 0;
        SearchTreeFlags flags = STF_NONE;

    };

    Position m_RootPos;
    std::vector<Node> m_Nodes;

    void serializeNode(std::ostream& stream, const Node& node, Position& pos, int indent = 0) const;

};

std::ostream& operator<<(std::ostream& stream, const SearchTree& tree);

class SearchTracer {
public:
    /**
     * Starts constructing a new tree. The expected depth serves as a hint
     * to preallocate memory for nodes.
     */
    void newTree(const Position& rootPos, int expectedDepth);
    inline std::shared_ptr<SearchTree> finishTree() {
        return std::move(m_SearchTree);
    }

    void push(Move move);
    void pop();

    inline void addFlags(SearchTreeFlags flags) {
        current().flags = SearchTreeFlags(current().flags | flags);
    }

    inline void setScores(i32 score, i32 alpha, i32 beta) {
        Node& curr = current();
        curr.score = score;
        curr.alpha = alpha;
        curr.beta  = beta;
    }

    inline void updateBestMove(Move move) {
        current().bestMove = move;
    }

    inline void setStaticEval(i32 eval) {
        current().staticEval = eval;
    }

    inline void setRequestedDepth(ui8 depth) {
        current().requestedDepth = depth;
    }

private:
    using Node = SearchTree::Node;

    std::shared_ptr<SearchTree> m_SearchTree = nullptr;
    ui32 m_CurrNodeIdx = 0;

    inline Node& current() { return m_SearchTree->m_Nodes[m_CurrNodeIdx]; }
    inline Node& parent()  { return m_SearchTree->m_Nodes[current().parentIndex]; }
    inline Node& root()    { return m_SearchTree->m_Nodes[0]; }
};

}

#endif // LUNA_SEARCHTRACE_H