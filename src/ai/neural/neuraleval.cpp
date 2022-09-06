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
    std::memset(pieces, 0, sizeof(pieces));

    // Pieces

    for (Square s = 0; s < 64; ++s) {
        Piece p = pos.getPieceAt(s);

        int idx;
        if (us == CL_WHITE) {
            idx = s;
        }
        else {
            // Mirror squares when placing on black's perspective
            auto rank = getRank(s);
            auto file = getFile(s);
            idx = getSquare(file, RANK_8 - rank);
        }

        pieces[idx] = p.getType();
        if (p.getColor() != us) {
            pieces[idx] *= -1;
        }
    }

    // Castling rights
    castleRightsUs = 0;
    if (pos.getCastleRights(us, SIDE_KING)) {
        castleRightsUs += 1;
    }
    if (pos.getCastleRights(us, SIDE_QUEEN)) {
        castleRightsUs += 2;
    }

    castleRightsThem = 0;
    if (pos.getCastleRights(getOppositeColor(us), SIDE_KING)) {
        castleRightsThem += 1;
    }
    if (pos.getCastleRights(getOppositeColor(us), SIDE_QUEEN)) {
        castleRightsThem += 2;
    }

    // En passant square
    epSquare = pos.getEnPassantSquare();
}

};

