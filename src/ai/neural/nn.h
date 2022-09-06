#ifndef LUNA_NEURAL_NN_H
#define LUNA_NEURAL_NN_H

#include <array>
#include <memory>

#include <nlohmann/json.hpp>

#include "../../position.h"
#include "../../utils.h"

namespace lunachess::ai::neural {

extern int n;

enum class ActivationFunctionType {
    None,
    ReLu
};

template <int N, int INPUT_SIZE, ActivationFunctionType ACT_FN_TYPE = ActivationFunctionType::ReLu>
struct NetworkLayer {
    std::array<std::array<float, INPUT_SIZE>, N> weights;
    std::array<float, N> biases;

    void randomize(float minValue = -1.0f, float maxValue = 1.0f) {
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < INPUT_SIZE; ++j) {
                weights[i][j] = utils::random(minValue, maxValue);
            }
            biases[i] = utils::random(minValue, maxValue);
        }
    }

    void mutate(float mutationRatePct) {
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < INPUT_SIZE; ++j) {
                if (utils::randomChance(mutationRatePct / 2)) {
                    weights[i][j] += utils::random(-0.1f, 0.1f);
                }

                if (utils::randomChance(mutationRatePct / 2)) {
                    weights[i][j] *= utils::random(0.95f, 1.05f);
                }
            }
            if (utils::randomChance(mutationRatePct / 2)) {
                biases[i] += utils::random(-0.1f, 0.1f);
            }
            if (utils::randomChance(mutationRatePct / 2)) {
                biases[i] *= utils::random(0.95f, 1.05f);
            }
        }
    }

    void feedForward(const float* source, float* dest) const {
        for (int i = 0; i < N; ++i) {
            float neuronSum = 0;
            for (int j = 0; j < INPUT_SIZE; ++j) {
                neuronSum += source[j] * weights[i][j];
            }
            dest[i] = activationFunction(neuronSum + biases[i]);
        }
    }

    NetworkLayer() = default;
    NetworkLayer(const NetworkLayer<N, INPUT_SIZE, ACT_FN_TYPE>& a,
                 const NetworkLayer<N, INPUT_SIZE, ACT_FN_TYPE>& b) {
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < INPUT_SIZE; ++j) {
                weights[i][j] = (utils::randomBool()) ? a.weights[i][j] : b.weights[i][j];
            }
            biases[i] = (utils::randomBool()) ? a.biases[i] : b.biases[i];
        }
    }

    static float activationFunction(float x) {
        if constexpr (ACT_FN_TYPE == ActivationFunctionType::ReLu) {
            return std::max(0.0f, x);
        }
        return x;
    }
};

template <int N_INPUTS,
        int N_HIDDEN_NEURONS,
        int N_HIDDEN_LAYERS>
struct NeuralNetwork {
    static constexpr int N_IN_BETWEEN_LAYERS = N_HIDDEN_LAYERS - 1;

    using TFirstHidden = NetworkLayer<N_HIDDEN_NEURONS, N_INPUTS>;
    using THidden      = NetworkLayer<N_HIDDEN_NEURONS, N_HIDDEN_NEURONS>;
    using TOut         = NetworkLayer<1, N_HIDDEN_NEURONS, ActivationFunctionType::None>;

    TFirstHidden firstHidden;
    TOut         outputLayer;
    std::array<THidden, N_IN_BETWEEN_LAYERS> inBetweenLayers;

    float evaluate(const float* arr) const {
        float a[N_HIDDEN_NEURONS], b[N_HIDDEN_NEURONS];
        float* src = a;
        float* dst = b;

        // Feed to first hidden layers
        firstHidden.feedForward(arr, a);

        // Subsequently, feed to next layers
        for (int i = 0; i < N_IN_BETWEEN_LAYERS; ++i) {
            inBetweenLayers[i].feedForward(src, dst);
            std::swap(src, dst);
        }

        // Feed to the output layer
        outputLayer.feedForward(src, dst);

        return dst[0];
    }

    void randomize(float minValue = -1, float maxValue = 1) {
        firstHidden.randomize(minValue, maxValue);
        for (int i = 0; i < N_IN_BETWEEN_LAYERS; ++i) {
            inBetweenLayers[i].randomize(minValue, maxValue);
        }
        outputLayer.randomize(minValue, maxValue);
    }

    void mutate(float mutationRatePct) {
        firstHidden.mutate(mutationRatePct);
        for (int i = 0; i < N_IN_BETWEEN_LAYERS; ++i) {
            inBetweenLayers[i].mutate(mutationRatePct);
        }
        outputLayer.mutate(mutationRatePct);
    }

    NeuralNetwork() = default;
    NeuralNetwork(const NeuralNetwork&) = default;
    NeuralNetwork(NeuralNetwork&&) noexcept = default;
    NeuralNetwork& operator=(const NeuralNetwork&) = default;

    inline NeuralNetwork(const NeuralNetwork<N_INPUTS, N_HIDDEN_NEURONS, N_HIDDEN_LAYERS>& a,
                         const NeuralNetwork<N_INPUTS, N_HIDDEN_NEURONS, N_HIDDEN_LAYERS>& b) {
        new (&firstHidden) TFirstHidden (a.firstHidden, b.firstHidden);
        for (int i = 0; i < N_IN_BETWEEN_LAYERS; ++i) {
            new (&inBetweenLayers[i]) THidden (a.inBetweenLayers[i], b.inBetweenLayers[i]);
        }
        new (&outputLayer) TOut (a.outputLayer, b.outputLayer);
    }

    ~NeuralNetwork() = default;
};

} // lunachess::ai::neural

#endif // LUNA_NEURAL_NN_H