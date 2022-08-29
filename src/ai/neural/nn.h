#ifndef LUNA_NEURAL_NN_H
#define LUNA_NEURAL_NN_H

#include <memory>

#include "../../position.h"
#include "../../utils.h"

namespace lunachess::ai::neural {

template <int N, int INPUT_SIZE>
class NetworkLayer {
public:
    void randomize(float minValue = -1.0f, float maxValue = 1.0f) {
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < INPUT_SIZE; ++j) {
                m_Weights[i][j] = utils::random(minValue, maxValue);
            }
            m_Biases[i] = utils::random(minValue, maxValue);
        }
    }

    static NetworkLayer crossover(const NetworkLayer& a, const NetworkLayer& b) {
        NetworkLayer result;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < INPUT_SIZE; ++j) {
                result.m_Weights[i][j] = (utils::randomBool()) ? a.m_Weights[i][j] : b.m_Weights[i][j];
            }
            result.m_Biases[i] = (utils::randomBool()) ? a.m_Biases[i] : b.m_Biases[i];
        }
        return result;
    }

    void mutate(int mutationRatePct) {
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < INPUT_SIZE; ++j) {
                if (utils::randomChance(mutationRatePct)) {
                    m_Weights[i][j] += utils::random(-0.1f, 0.1f);
                }
            }
            if (utils::randomChance(mutationRatePct)) {
                m_Biases[i] += utils::random(-0.1f, 0.1f);
            }
        }
    }

    void feedForward(const float* source, float* dest) const {
        for (int i = 0; i < N; ++i) {
            float neuronSum = 0;
            for (int j = 0; j < INPUT_SIZE; ++j) {
                neuronSum += source[j] * m_Weights[i][j];
            }
            dest[i] = activationFunction(neuronSum + m_Biases[i]);
        }
    }


private:
    float m_Weights[N][INPUT_SIZE];
    float m_Biases[N];

    static float activationFunction(float x) {
        return std::max(0.0f, x);
    }
};

template <int N_INPUTS,
        int N_HIDDEN_NEURONS,
        int N_HIDDEN_LAYERS>
class NeuralNetwork {
    static constexpr int N_INBETWEEN_LAYERS = N_HIDDEN_LAYERS - 1;

    using TFirstHidden = NetworkLayer<N_HIDDEN_NEURONS, N_INPUTS>;
    using THidden      = NetworkLayer<N_HIDDEN_NEURONS, N_HIDDEN_NEURONS>;
    using TOut         = NetworkLayer<1, N_HIDDEN_NEURONS>;
public:

    float evaluate(const float* arr) const {
        float a[N_HIDDEN_NEURONS], b[N_HIDDEN_NEURONS];
        float* src = a;
        float* dst = b;

        // Feed to first hidden layers
        m_FirstHidden.feedForward(arr, a);

        // Subsequently feed to next layers
        for (int i = 0; i < N_INBETWEEN_LAYERS; ++i) {
            m_HiddenLayers[i].feedForward(src, dst);
            std::swap(src, dst);
        }

        // Feed to the output layer
        m_OutputLayer.feedForward(src, dst);

        return dst[0];
    }

    void randomize(float minValue = -1, float maxValue = 1) {
        m_FirstHidden.randomize(minValue, maxValue);
        for (int i = 0; i < N_INBETWEEN_LAYERS; ++i) {
            m_HiddenLayers[i].randomize(minValue, maxValue);
        }
        m_OutputLayer.randomize(minValue, maxValue);
    }

    void mutate(int mutationRatePct) {
        m_FirstHidden.mutate(mutationRatePct);
        for (int i = 0; i < N_INBETWEEN_LAYERS; ++i) {
            m_HiddenLayers[i].mutate(mutationRatePct);
        }
        m_OutputLayer.mutate(mutationRatePct);
    }

    static NeuralNetwork crossover(const NeuralNetwork& a, const NeuralNetwork& b) {
        NeuralNetwork ret;

        ret.m_FirstHidden = TFirstHidden::crossover(a.m_FirstHidden, b.m_FirstHidden);
        for (int i = 0; i < N_INBETWEEN_LAYERS; ++i) {
            ret.m_HiddenLayers[i] = THidden::crossover(a.m_HiddenLayers[i], b.m_HiddenLayers[i]);
        }
        ret.m_OutputLayer = TOut::crossover(a.m_OutputLayer, b.m_OutputLayer);

        return ret;
    }

private:
    TFirstHidden m_FirstHidden;
    THidden      m_HiddenLayers[N_INBETWEEN_LAYERS];
    TOut         m_OutputLayer;
};

} // lunachess::ai::neural

#endif // LUNA_NEURAL_NN_H
