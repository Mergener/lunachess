#include "searchtrace.h"

#include <iomanip>

namespace lunachess::ai {

void SearchTracer::newTree(const Position& pos, int expectedDepth) {
    // Reference position for Luna 1.0 dev-5:
    // r1b2rk1/1pp2p1p/2np1np1/pB2b1q1/4P3/NP3N2/P1PB1PPP/2RQ1K1R b - - 4 11
    constexpr int DEPTH_SIZE_TABLE[] {
        1, 250, 450, 1300, 2100, 5000, 11000,
        25000, 50000, 90000, 165000, 275000, 450000,
        875000, 1550000,
    };

    int expectedNodes = DEPTH_SIZE_TABLE[std::min(expectedDepth, int(sizeof(DEPTH_SIZE_TABLE)/sizeof(*DEPTH_SIZE_TABLE)))];
    m_SearchTree = std::make_shared<SearchTree>(pos, expectedNodes);

    // Add root node
    m_SearchTree->m_Nodes.emplace_back();
}

void SearchTracer::push(Move move) {
    for (ui32 i = 0; i < current().nChildren; ++i) {
        ui32 childIdx = current().childIndexes[i];
        auto& child = m_SearchTree->m_Nodes[childIdx];

        if (child.lastMove == move) {
            // A child with the same move already existed. Overwrite it.
            m_CurrNodeIdx = childIdx;
            return;
        }
    }

    // Create the node in the tree
    ui32 parentIdx  = m_CurrNodeIdx;
    ui32 newNodeIdx = m_SearchTree->m_Nodes.size();
    m_SearchTree->m_Nodes.emplace_back(); // adds empty node

    // Reference it
    Node& parent = current();
    m_CurrNodeIdx = newNodeIdx;
    Node& curr = current();
    curr.lastMove = move;

    // Set correct values
    curr.parentIndex = parentIdx;
    parent.childIndexes[parent.nChildren++] = newNodeIdx;
}

void SearchTracer::pop() {
    m_CurrNodeIdx = current().parentIndex;
}

std::ostream& operator<<(std::ostream& stream, const SearchTree& tree) {
    const SearchTree::Node& rootNode = tree.m_Nodes[0];
    Position pos = tree.m_RootPos;

    stream << std::setw(2);
    stream << std::boolalpha;

    tree.serializeNode(stream, rootNode, pos);

    return stream;
}

struct Indent {
    inline Indent(int i): indent(i) {}

    int indent;
};
inline std::ostream& operator<<(std::ostream& stream, Indent indent) {
//    int width = stream.width();
    int width = 2;
    if (width == 0) {
        return stream;
    }
    for (int i = 0; i < indent.indent * width; ++i) {
        stream << ' ';
    }
    return stream;
}

template <typename T>
struct Field {
    inline Field(std::string name, T value, bool last = false)
        : name(std::move(name)), value(std::move(value)), last(last) {}

    std::string name;
    T value;
    bool last = false;
};

template <typename T>
inline std::ostream& operator<<(std::ostream& stream, const Field<T>& field) {
    stream << '"' << field.name << "\": ";

    constexpr bool DISPLAY_AS_STRING = std::is_same_v<T, std::string> ||
            std::is_same_v<T, Move>;
    if constexpr (DISPLAY_AS_STRING) {
        stream << '"';
    }
    stream << field.value;
    if constexpr (DISPLAY_AS_STRING) {
        stream << '"';
    }

    if (!field.last) {
        stream << ',';
    }

    return stream;
}

void SearchTree::serializeNode(std::ostream& stream, const Node& node, Position& pos, int indent) const {
    stream << "{" << '\n';
    indent++;

    // Get best move
    stream << Indent(indent) << Field("fen", pos.toFen()) << '\n';
    stream << Indent(indent) << Field<int>("requestedDepth", node.requestedDepth) << '\n';
    stream << Indent(indent) << Field("score", node.score) << '\n';
    stream << Indent(indent) << Field("staticEval", node.staticEval) << '\n';
    stream << Indent(indent) << Field("alpha", node.alpha) << '\n';
    stream << Indent(indent) << Field("beta", node.beta) << '\n';
    if (node.bestMove != MOVE_INVALID) {
        stream << Indent(indent) << Field<Move>("bestMove", node.bestMove) << '\n';
    }
    stream << Indent(indent) << Field<bool>("betaCutoff", BIT_INTERSECTS(node.flags, STF_BETA_CUTOFF)) << '\n';
    stream << Indent(indent) << Field<bool>("pv", BIT_INTERSECTS(node.flags, STF_PV)) << '\n';

    stream << Indent(indent) << "\"children\": {" << '\n';
    indent++;
    for (ui32 i = 0; i < node.nChildren; ++i) {
        ui32 childIdx     = node.childIndexes[i];
        const Node& child = m_Nodes[childIdx];
        Move move         = child.lastMove;

        stream << Indent(indent) << '"' << move << "\": ";
        if (move != MOVE_INVALID) {
            pos.makeMove(move);
        }
        else {
            pos.makeNullMove();
        }

        serializeNode(stream, child, pos, indent);

        if (move != MOVE_INVALID) {
            pos.undoMove();
        }
        else {
            pos.undoNullMove();
        }

        if (i < node.nChildren - 1) {
            stream << ','; // Commma for non-last childs
        }
        stream << '\n';
    }
    indent--;
    stream << Indent(indent) << "}\n";

    indent--;
    stream << Indent(indent) << "}";
}


}
