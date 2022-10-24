#ifndef LUNA_NEURAL_NN_H
#define LUNA_NEURAL_NN_H

#include <array>
#include <memory>
#include <cstring>
#include <immintrin.h>

#include <nlohmann/json.hpp>

#include "../../position.h"
#include "../../utils.h"

namespace lunachess::ai::neural {

enum class ActivationFunctionType {
    ReLu
};

inline static constexpr int ceilToMultiple(int n, int mult) {
    return (n % mult == 0)
        ? n
        : (n + mult - (n % mult));
}

/**
 * Represents a single layer of an evaluation neural network.
 * @tparam N The number of neurons in this layer.
 * @tparam N_INPUTS The number of expected input values for this layer.
 * @tparam ACT_FN_TYPE The type of activation function to be used when propagating.
 */
template <int N_NEURONS, int N_INPUT_NEURONS,
          ActivationFunctionType ACT_FN_TYPE = ActivationFunctionType::ReLu>
struct NNLayer {
    static constexpr int N_PER_CHUNK = 8;
    static constexpr int N           = N_NEURONS;
    static constexpr int N_INPUTS    = ceilToMultiple(N_INPUT_NEURONS, N_PER_CHUNK);

    std::array<std::array<i32, N_INPUTS>, N> weights;
    std::array<i32, N> biases;

    inline static __m256i activationFunction(const __m256i& x) {
        if constexpr (ACT_FN_TYPE == ActivationFunctionType::ReLu) {
            __m256i max = _mm256_set1_epi32(0);
            return _mm256_max_epi32(max, x);
        }
        return x;
    }

    void propagate(const std::array<i32, N_INPUTS>& inputs,
                   std::array<i32, N>& outputs) const {
        const __m256i* wPtr  = weights.data();
        const __m256i* inPtr = inputs.data();
        __m256i* outPtr      = outputs.data();

        for (int i = 0; i < N; ++i) {
            __m256i sum = _mm256_set1_epi32(biases[i]);
            for (int j = 0; j < N_INPUTS; j += N_PER_CHUNK) {
                __m256i iw = _mm256_mullo_epi32(*wPtr, *inPtr);
                sum = _mm256_add_epi32(sum, iw);
                inPtr++; wPtr++;
            }
            *outPtr = activationFunction(sum);
            outPtr++;
        }
    }

    void fillWeights(i32 val) {
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N_INPUT_NEURONS; ++j) {
                weights[i][j] = val;
            }
        }
    }

    void fillBiases(i32 val) {
        std::fill(biases.begin(), biases.end(), val);
    }

    NNLayer() noexcept {
        std::memset(reinterpret_cast<void*>(this), 0, sizeof(*this));
    }

    NNLayer(const NNLayer& other) = default;
    NNLayer& operator=(const NNLayer& other) = default;
    NNLayer(NNLayer&& other) = default;
    ~NNLayer() = default;
};

template <int N, int N_INPUTS, ActivationFunctionType ACT_FN_TYPE = ActivationFunctionType::ReLu>
void to_json(nlohmann::json& j, const NNLayer<N, N_INPUTS, ACT_FN_TYPE>& layer) {
    // Save weights
    static constexpr size_t SIZE_BYTES = N_INPUTS * N * sizeof(__m256i);
    std::vector<i32> weights(SIZE_BYTES / sizeof(i32));
    std::memcpy(weights.data(), layer.weights.data(), SIZE_BYTES);
    j["weights"] = weights;

    // Save biases
    j["biases"] = layer.biases;
}

template <int N, int N_INPUTS, ActivationFunctionType ACT_FN_TYPE = ActivationFunctionType::ReLu>
void from_json(const nlohmann::json& j, NNLayer<N, N_INPUTS, ACT_FN_TYPE>& layer) {
    static constexpr size_t SIZE_BYTES = N_INPUTS * N * sizeof(__m256i);
    std::vector<i32> weights = j["weights"];
    std::memcpy(layer.weights.data(), weights.data(), SIZE_BYTES);

    layer.biases = j["biases"];
}

struct EvalNN {

};

} // lunachess::ai::neural

#endif // LUNA_NEURAL_NN_H
