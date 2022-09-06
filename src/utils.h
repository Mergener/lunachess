#ifndef LUNA_UTILS_H
#define LUNA_UTILS_H

#include <string>
#include <sstream>
#include <functional>
#include <vector>

#include "types.h"

namespace lunachess::utils {

template <typename T>
std::string toString(const T& val) {
    std::stringstream stream;
    stream << val;
    return stream.str();
}

i64 random(i64 minInclusive, i64 maxExclusive);
ui64 random(ui64 minInclusive, ui64 maxExclusive);
i32 random(i32 minInclusive, i32 maxExclusive);
ui32 random(ui32 minInclusive, ui32 maxExclusive);
float random(float min, float max);
double random(double min, double max);

inline float randomFloat01() { return random(0.0f, 1.0f); }
inline double randomDouble01() { return random(0.0, 1.0); }

inline bool randomBool() {
    return random(0, 2);
}

inline bool randomChance(int chancePct) {
    return random(1, 101) <= chancePct;
}

inline bool randomChance(float chancePct) {
    float rnd = random(1.0f, 100.0f);

    return rnd <= chancePct;
}

template <typename T, typename TNew>
std::vector<TNew> mapTo(const std::vector<T>& src, std::function<TNew(const T&)> fn) {
    std::vector<TNew> ret;
    ret.reserve(src.capacity());

    for (const auto& val: src) {
        ret.push_back(fn(val));
    }

    return ret;
}


} // lunachess::utils

#endif // LUNA_UTILS_H