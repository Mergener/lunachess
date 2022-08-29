#include "utils.h"

#include <random>
#include <mutex>
#include <type_traits>

#include "types.h"

namespace lunachess::utils {

using RandomEngine = std::mt19937_64;
static RandomEngine s_Random (std::random_device{}());
static std::mutex s_RandomMutex;

template <typename TInt>
TInt randomInteger(TInt minInclusive, TInt maxExclusive) {
    static_assert(std::is_integral_v<TInt>);

    std::unique_lock lock(s_RandomMutex);
    i64 rnd = static_cast<i64>(s_Random());

    i64 delta = maxExclusive - minInclusive;
    return rnd % delta + minInclusive;
}

template <typename TFloat>
TFloat randomFloatingPoint(TFloat min, TFloat max) {
    static_assert(std::is_floating_point_v<TFloat>);

    std::unique_lock lock(s_RandomMutex);
    i64 rnd = static_cast<i64>(s_Random());

    // The following line gives us a random float between 0 and 1
    TFloat f01 = static_cast<TFloat>(rnd) / static_cast<TFloat>(RandomEngine::max());

    // Now, adjust it to the provided range and return
    TFloat delta = max - min;
    return f01 * delta + min;
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