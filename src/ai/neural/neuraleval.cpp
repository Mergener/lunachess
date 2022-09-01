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
    return static_cast<int>(m_Network->evaluate(inputs) * 100.0f);
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
        pieces[s] = p.getType();
        if (p.getColor() != us) {
            pieces[s] *= -1;
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

class Agent {
public:

    inline void addScore() { m_Score++; }
    inline void reduceScore() { m_Score--; }

    inline const NeuralEvaluator& getEvaluator() const { return m_Eval; }

    Agent();
    ~Agent() = default;

private:
    NeuralEvaluator m_Eval;
    int m_Score = 0;
};


};

