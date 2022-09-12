#include "neuraleval.h"

#include <algorithm>
#include <cstring>
#include <mmintrin.h>
#include <random>
#include <vector>
#include "../../position.h"

namespace lunachess::ai::neural {

static std::mt19937_64 s_Random (std::random_device{}());

int NeuralEvaluator::evaluate(const NeuralInputs& inputs) const {
    return static_cast<int>(m_Network->evaluate(inputs));
}

int NeuralEvaluator::evaluate(const Position& pos) const {
    NeuralInputs inputs(pos);
    return evaluate(inputs);
}

NeuralInputs::NeuralInputs(const Position& pos, Color us) {
    zeroAll();

    // Pieces
    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
        for (PieceType pt = PT_PAWN; pt < PT_COUNT; ++pt) {
            Piece p = Piece(c, pt);
            Bitboard bb = pos.getBitboard(p);
            
            for (Square s: bb) {
                int arrIdx = c == us ? 0 : 1;
                Square sqIdx = us == CL_WHITE ? s : getSquare(FL_COUNT - getFile(s) - 1, getRank(s));
                pieceMaps[arrIdx][pt - 1][sqIdx] = 1.0f;
            }
        }
    }

    // Castling rights
    if (pos.getCastleRights(us, SIDE_KING)) {
        weCanCastleShort = 1.0f;
    }
    if (pos.getCastleRights(us, SIDE_QUEEN)) {
        weCanCastleLong = 1.0f;
    }

    if (pos.getCastleRights(getOppositeColor(us), SIDE_KING)) {
        theyCanCastleShort = 1.0f;
    }
    if (pos.getCastleRights(getOppositeColor(us), SIDE_QUEEN)) {
        theyCanCastleLong = 1.0f;
    }
}

};

