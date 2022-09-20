#ifndef NEURALEVAL_H
#define NEURALEVAL_H

#include <cstring>
#include <array>

#include "../evaluator.h"
#include "../../position.h"

#include "nn.h"

namespace lunachess::ai::neural {

struct NeuralInputs {
    // Index 0 is us, index 2 is them
    // We subtract PT_COUNT by 1 to discard PT_NONE
    std::array<std::array<std::array<float, 64>, PT_COUNT - 1>, CL_COUNT> pieceMaps;

    float weCanCastleShort;
    float theyCanCastleShort;
    float weCanCastleLong;
    float theyCanCastleLong;

    inline operator float*() {
        return std::launder(reinterpret_cast<float*>(this));
    }

    inline operator const float*() const {
        return std::launder(reinterpret_cast<const float*>(this));
    }

    inline void zeroAll() {
        std::memset(reinterpret_cast<void*>(this), 0, sizeof(*this));
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
    using NN = NeuralNetwork<N_INPUTS, 32, 1>;

    inline NN& getNetwork() { return *m_Network; }
    inline const NN& getNetwork() const { return *m_Network; }

    inline int getDrawScore() const override { return 0; }
    int evaluate(const Position& pos) const override;
    int evaluate(const NeuralInputs& inputs) const;

    /**
     * Creates a neural evaluator and uses a specified neural network.
     */
    inline NeuralEvaluator(std::shared_ptr<NN> nn)
        : m_Network(nn) {
    }

    /**
     * Creates a neural evaluator and an underlying neural network.
     */
    inline NeuralEvaluator()
            : m_Network(std::make_shared<NN>()) {
    }

    inline NeuralEvaluator(const NeuralEvaluator& other)
        : m_Network(std::make_shared<NN>(*other.m_Network)) {}

    inline NeuralEvaluator(NeuralEvaluator&& other)
        : m_Network(std::move(other.m_Network)) {}
    NeuralEvaluator& operator=(const NeuralEvaluator&) = default;

    ~NeuralEvaluator() = default;

private:
    std::shared_ptr<NN> m_Network;
};

}

#endif //NEURALEVAL_H