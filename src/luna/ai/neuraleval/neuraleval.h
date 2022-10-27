#ifndef NEURALEVAL_H
#define NEURALEVAL_H

#include <cstring>
#include <array>

#include "../evaluator.h"
#include "../../position.h"
#include "../../utils.h"

#include "nnlayer.h"

namespace lunachess::ai::neural {

//#pragma pack(push, LUNA_NN_STRIDE)
struct NNInputs {
    i32 psqt[CL_COUNT][PT_COUNT - 1][SQ_COUNT];
    i32 castleRights[CL_COUNT][SIDE_COUNT];
    i32 colorToMove;

    void fromPosition(const Position& pos);
    void updateMakeMove(Move move);
    void updateUndoMove(Move move);
    void updateMakeNullMove();
    void updateUndoNullMove();

    inline NNInputs() {
        utils::zero(*this);
    }
};
//#pragma pack(pop)

constexpr int INPUT_SIZE = CL_COUNT * (PT_COUNT - 1) * SQ_COUNT + // psqt
                           CL_COUNT * SIDE_COUNT +                // castleRights
                           1;                                     // colorToMove

using W1Layer = NNLayer<32, INPUT_SIZE>;
using W2Layer = NNLayer<16, W1Layer::N_NEURONS>;
using W3Layer = NNLayer<16, W2Layer::N_NEURONS>;
using W4Layer = NNLayer<1,  W3Layer::N_NEURONS>;

struct EvalNN {
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EvalNN, w1, w2, w3, w4);

    W1Layer w1;
    W2Layer w2;
    W3Layer w3;
    W4Layer w4;

    inline int evaluate(const NNInputs& inputs) {
        return evaluate(std::launder(reinterpret_cast<const i32*>(&inputs)));
    }

    int evaluate(const i32* arr);
};

class NeuralEvaluator : public Evaluator {
public:
    inline int getDrawScore() const override { return 0; }
    inline int evaluate(const Position& pos) const override {
        return m_NN->evaluate(*m_Accum);
    }

    inline void preparePosition(const Position& pos) override {
        m_Accum->fromPosition(pos);
    }

    inline void onMakeMove(Move move) const override {
        m_Accum->updateMakeMove(move);
    }

    inline void onUndoMove(Move move) const override {
        m_Accum->updateUndoMove(move);
    }

    inline void onMakeNullMove() const override {
        m_Accum->updateMakeNullMove();
    }

    inline void onUndoNullMove() const override {
        m_Accum->updateUndoNullMove();
    }

    /**
     * Creates a neuraleval evaluator and an underlying neuraleval network.
     */
    inline NeuralEvaluator()
        : m_NN(std::make_unique<EvalNN>()),
          m_Accum(std::make_unique<NNInputs>()) {}

    ~NeuralEvaluator() = default;

private:
    std::unique_ptr<EvalNN>   m_NN;
    std::unique_ptr<NNInputs> m_Accum;
};

}

#endif //NEURALEVAL_H