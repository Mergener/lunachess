#ifndef LUNA_AI_NNUE_H
#define LUNA_AI_NNUE_H

#include <array>

#include <immintrin.h>

#include "../../types.h"
#include "../../position.h"

namespace lunachess::ai::nnue {

static constexpr size_t N_FEATURES = 768;
static constexpr size_t L1_SIZE    = 32;
static constexpr size_t L2_SIZE    = 32;
static constexpr size_t OUT_SIZE   = 2;

static constexpr size_t ALIGN = 8;

template <size_t N_IN, size_t N_OUT>
struct alignas(ALIGN) Layer {
    std::array<std::array<i16, N_IN>, N_OUT> weights;
    std::array<i16, N_OUT> biases;
};

using L1 = Layer<N_FEATURES, L1_SIZE>;
using L2 = Layer<L1_SIZE, L2_SIZE>;
using L3 = Layer<L2_SIZE, OUT_SIZE>;
using FeaturesArray = std::array<i16, N_FEATURES>;

struct alignas(ALIGN) Accumulator {

    std::array<std::array<i16, L1_SIZE>, CL_COUNT> featuresSum;

    void refresh(const Position& pos, const i16* weights, const i16* biases);
    void update(Move move, const i16* weights, const i16* biases);

};

struct alignas(ALIGN) NNUE {

    L1 l1;
    L2 l2;
    L3 l3;

    std::tuple<i32, i132> propagate(const FeaturesArray& features,
                                    const Accumulator& accumulator);

};

#define USE_AVX2

template <size_t N_IN, size_t N_OUT>
void affineTransform(const i16* inputs, i16* outputs, const i16* weights, const i16* biases) {
#ifdef USE_AVX2
    for (size_t i = 0; i < N_OUT; ++i) {
        __m256i sum = _mm256_setzero_si256();

        for (size_t j = 0; j < N_IN; j += 16) {
            __m256i inputValues  = _mm256_loadu_si256((__m256i*)(inputs + j));
            __m256i weightValues = _mm256_loadu_si256((__m256i*)(weights + i * N_IN + j));

            __m256i product = _mm256_mullo_epi16(inputValues, weightValues);
            sum = _mm256_add_epi16(sum, product);
        }

        sum = _mm256_hadd_epi16(sum, sum);

        i32 result = _mm256_extract_epi32(sum, 0) + biases[i];
        outputs[i] = static_cast<i16>(result);
    }
#else
    for (size_t i = 0; i < N_OUT; ++i) {
        i32 sum = 0;

        for (size_t j = 0; j < N_IN; ++j) {
            i32 inputValue  = static_cast<i32>(inputs[j]);
            i32 weightValue = static_cast<i32>(weights[i * N_IN + j]);
            sum += inputValue * weightValue;
        }

        i32 result = sum + static_cast<int32_t>(biases[i]);
        outputs[i] = static_cast<i16>((result + (result >= 0 ? 32768 : -32768)) >> 16);
    }
#endif
}

template <size_t N_IN, size_t N_OUT, i16 CRELU_CAP>
void clippedReLU(const i16* inputs, i16* outputs, const i16* weights, const i16* biases) {
#ifdef USE_AVX2
    for (size_t i = 0; i < N_OUT; ++i) {
        __m256i sum = _mm256_setzero_si256();

        for (size_t j = 0; j < N_IN; j += 16) {
            __m256i inputValues = _mm256_loadu_si256((__m256i*)(inputs + j));
            __m256i weightValues = _mm256_loadu_si256((__m256i*)(weights + i * N_IN + j));

            __m256i product = _mm256_mullo_epi16(inputValues, weightValues);
            sum = _mm256_add_epi16(sum, product);
        }

        sum = _mm256_hadd_epi16(sum, sum);
        sum = _mm256_hadd_epi16(sum, sum);

        i32 result = _mm256_extract_epi32(sum, 0) + biases[i];

        i16 clippedResult = result > CRELU_CAP ? CRELU_CAP : (result < 0 ? 0 : static_cast<i16>(result));
        outputs[i] = clippedResult;
    }
#else
    for (size_t i = 0; i < N_OUT; ++i) {
        i32 sum = static_cast<int32_t>(biases[i]);

        for (size_t j = 0; j < N_IN; ++j) {
            i32 inputValue = static_cast<i32>(inputs[j]);
            i32 weightValue = static_cast<i32>(weights[i * N_IN + j]);
            sum += inputValue * weightValue;
        }

        i32 result = sum;

        i16 clippedResult = result > CRELU_CAP ? CRELU_CAP : (result < 0 ? 0 : static_cast<i16>(result));
        outputs[i] = clippedResult;
    }
#endif
}





}

#endif // LUNA_AI_NNUE_H
