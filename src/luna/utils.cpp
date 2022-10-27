#include "utils.h"

#include <random>
#include <mutex>
#include <type_traits>
#include <cstring>

#include "types.h"

namespace lunachess::utils {

using RandomEngine = std::mt19937_64;
static RandomEngine s_Random (std::random_device{}());
static std::mutex s_RandomMutex;

template <typename TInt>
TInt randomInteger(TInt minInclusive, TInt maxExclusive) {
    static_assert(std::is_integral_v<TInt>);

    std::uniform_int_distribution<TInt> gen(minInclusive, std::max(maxExclusive - 1, minInclusive));

    return gen(s_Random);
}

template <typename TFloat>
TFloat randomFloatingPoint(TFloat min, TFloat max) {
    constexpr ui64 MAX = 1000000000;
    static_assert(std::is_floating_point_v<TFloat>);

    std::uniform_real_distribution<TFloat> gen(min, max);

    return gen(s_Random);
}

float random(float min, float max) {
    return randomFloatingPoint(min, max);
}

double random(double min, double max) {
    return randomFloatingPoint(min, max);
}

i64 random(i64 minInclusive, i64 maxExclusive) {
    return randomInteger(minInclusive, maxExclusive);
}

ui64 random(ui64 minInclusive, ui64 maxExclusive) {
    return randomInteger(minInclusive, maxExclusive);
}

i32 random(i32 minInclusive, i32 maxExclusive) {
    return randomInteger(minInclusive, maxExclusive);
}

ui32 random(ui32 minInclusive, ui32 maxExclusive) {
    return randomInteger(minInclusive, maxExclusive);
}

}