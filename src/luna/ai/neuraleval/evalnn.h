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

#define LUNA_NN_STRIDE 4

/**
 * Represents a single layer of an evaluation neural network.
 * @tparam N The number of neurons in this layer.
 * @tparam N_INPUTS The number of expected input values for this layer.
 * @tparam ACT_FN_TYPE The type of activation function to be used when propagating.
 */
#pragma pack(push, LUNA_NN_STRIDE)
template <int N, int N_INPUTS,
          ActivationFunctionType ACT_FN_TYPE = ActivationFunctionType::ReLu>
struct NNLayer {
    /** Input array length, including padding necessary for SIMD. */
    static constexpr int INPUT_ARRAY_SIZE = ceilToMultiple(N_INPUTS, LUNA_NN_STRIDE);

    using InputArray  = std::array<i32, INPUT_ARRAY_SIZE>;
    using OutputArray = std::array<i32, N>;

    std::array<std::array<i32, INPUT_ARRAY_SIZE>, N> weights;
    std::array<i32, N> biases;

    inline static i32 activationFunction(i32 x) {
        if constexpr (ACT_FN_TYPE == ActivationFunctionType::ReLu) {
            return std::max(x, 0);
        }
        return x;
    }

    void propagate(const std::array<i32, INPUT_ARRAY_SIZE>& inputs,
                   std::array<i32, N>& outputs) const {
        const __m128i* wVec = reinterpret_cast<const __m128i*>(weights.data());
        __m128i* outVec     = reinterpret_cast<__m128i*>(outputs.data());

        for (int i = 0; i < N; ++i) {
            const __m128i* inVec = reinterpret_cast<const __m128i*>(inputs.data());

            __m128i sum = _mm_set1_epi32(biases[i]);

            for (int j = 0; j < N_INPUTS; j += LUNA_NN_STRIDE) {
                __m128i in   = _mm_load_si128(inVec);
                __m128i w    = _mm_load_si128(wVec);
                __m128i prod = _mm_mullo_epi32(in, w);
                sum          = _mm_add_epi32(sum, prod);
                inVec++; wVec++;
            }

            sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
            sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
            outputs[i] = activationFunction(_mm_cvtsi128_si32(sum) >> 6);
            outVec++;
        }
    }

    void fillWeights(i32 val) {
        for (auto& w: weights) {
            std::fill(w.begin(), w.begin() + N_INPUTS, val);
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

struct NNInputs {
    enum {
        IDX_OO_US = SQ_COUNT,
        IDX_OOO_US,
        IDX_OO_THEM,
        IDX_OOO_THEM
    };

    enum {
        NO_PIECE,
        OUR_PAWN,
        OUR_KNIGHT,
        OUR_BISHOP,
        OUR_ROOK,
        OUR_QUEEN,
    };

    std::array<i32, SQ_COUNT + 4> data;

    void fromPosition(Position& pos);
    void updateMakeMove(Move move);
    void updateUndoMove(Move move);
    void updateMakeNullMove();
    void updateUndoNullMove();
};
#pragma pack(pop)

template <int N, int N_INPUTS, ActivationFunctionType ACT_FN_TYPE = ActivationFunctionType::ReLu>
void to_json(nlohmann::json& j, const NNLayer<N, N_INPUTS, ACT_FN_TYPE>& layer) {
    // Do not save padding weights
    std::array<std::array<i32, N_INPUTS>, N> weights;
    for (int i = 0; i < weights.size(); ++i) {
        for (int j = 0; j < N_INPUTS; ++j) {
            weights[i][j] = layer.weights[i][j];
        }
    }
    j["weights"] = weights;
    j["biases"]  = layer.biases;
}

template <int N, int N_INPUTS, ActivationFunctionType ACT_FN_TYPE = ActivationFunctionType::ReLu>
void from_json(const nlohmann::json& j, NNLayer<N, N_INPUTS, ACT_FN_TYPE>& layer) {
    std::vector<std::vector<i32>> weights = j["weights"];
    // Make sure padding weights are set to 0
    std::memset(reinterpret_cast<void*>(layer.weights.data()), 0, sizeof(layer.weights));
    for (int i = 0; i < weights.size(); ++i) {
        for (int j = 0; j < N_INPUTS; ++j) {
            layer.weights[i][j] = weights[i][j];
        }
    }
    
    layer.biases = j["biases"];
}

} // lunachess::ai::neural

#endif // LUNA_NEURAL_NN_H
