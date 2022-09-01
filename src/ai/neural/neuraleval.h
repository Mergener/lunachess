#ifndef NEURALEVAL_H
#define NEURALEVAL_H

#include "../evaluator.h"
#include "../../position.h"

#include "nn.h"

namespace lunachess::ai::neural {

struct NeuralInputs {
    float pieces[64]; // 0 for no pieces at square, >0 for our pieces, <0 for opponent pieces

    float castleRightsUs; // 0 for no castling rights, 1 for O-O, 2 for O-O-O, 3 for O-O and O-O-O
    float castleRightsThem; // same as above

    float epSquare;

    inline operator float*() {
        return std::launder(reinterpret_cast<float*>(this));
    }

    inline operator const float*() const {
        return std::launder(reinterpret_cast<const float*>(this));
    }

    NeuralInputs() = default;
    ~NeuralInputs() = default;

    inline NeuralInputs(const Position& pos)
        : NeuralInputs(pos, pos.getColorToMove()) {
    }

    NeuralInputs(const Position& pos, Color us);
};

class NeuralEvaluator : public Evaluator {
    static inline constexpr int N_INPUTS = sizeof(NeuralInputs) / sizeof(float);

public:
    using NN = NeuralNetwork<N_INPUTS, 2, 64>;

    inline NN& getNetwork() { return *m_Network; }
    inline const NN& getNetwork() const { return *m_Network; }

    inline int getDrawScore() const override { return 0; }
    int evaluate(const Position& pos) const override;
    int evaluate(const NeuralInputs& inputs) const;

    /**
     * Creates a neural evaluator and uses a specified neural network.
     */
    inline NeuralEvaluator(std::shared_ptr<NN> nn)
        : m_Network(nn) {}

    /**
     * Creates a neural evaluator and an underlying neural network.
     */
    inline NeuralEvaluator()
            : NeuralEvaluator(std::make_shared<NN>()) {}

    ~NeuralEvaluator() = default;

private:
    std::shared_ptr<NN> m_Network;
};

}

#endif //NEURALEVAL_H