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

    std::uniform_int_distribution<TInt> gen(minInclusive, std::max(maxExclusive - 1, minInclusive));

    return gen(s_Random);
}

template <typename TFloat>
TFloat randomFloatingPoint(TFloat min, TFloat max) {
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

// Taken from: https://stackoverflow.com/a/58467162
std::string randomUUID() {
    static std::random_device dev;
    static std::mt19937 rng(dev());

    std::uniform_int_distribution<int> dist(0, 15);

    const char* v = "0123456789abcdef";
    const bool dash[] = { 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0 };

    std::string res;
    for (int i = 0; i < 16; i++) {
        if (dash[i]) res += "-";
        res += v[dist(rng)];
        res += v[dist(rng)];
    }
    return res;
}


}