#ifndef LUNA_CLOCK_H
#define LUNA_CLOCK_H

#include <chrono>

#include "types.h"

namespace lunachess {

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock, Clock::duration>;

inline i64 deltaMs(TimePoint later, TimePoint earlier) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(later - earlier).count();
}

} // lunachess

#endif // LUNA_CLOCK_H