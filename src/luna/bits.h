#ifndef LUNA_BITS_H
#define LUNA_BITS_H

#include "types.h"

#ifdef __GNUC__
#include <cpuid.h>
#elif defined(_MSC_VER)
#include <intrin.h>
    #pragma intrinsic(_BitScanForward64)
    #pragma intrinsic(_BitScanReverse64)
#endif

#define BIT(n) (C64(1) << (n))
#define BIT_INTERSECTS(bitmask, bit) ((bitmask & bit) != 0)
#define BITMASK(nbits) ((1ul << (nbits)) - 1ul)

namespace lunachess::bits {

inline ui64 popcount(ui64 x) {
#ifdef __GNUC__
    return __builtin_popcountll(x);
#elif defined(_MSC_VER)
    return __popcnt64(x);
#endif
}

inline ui64 checkedPopcount(ui64 x) {
    return x != 0 ? popcount(x) : 0;
}

template <ui64 val>
inline constexpr ui64 makeMask() {
    return BIT(val);
}

template <ui64 val1, ui64 val2, ui64... vals>
inline constexpr ui64 makeMask() {
    return BIT(val1) | makeMask<val2, vals...>();
}

inline constexpr ui8 rotateLeft(ui8 val, ui8 rot) {
    return (val << rot) | (val >> ((sizeof(val) * 8) - rot));
}

inline constexpr ui16 rotateLeft(ui16 val, ui16 rot) {
    return (val << rot) | (val >> ((sizeof(val) * 8) - rot));
}

inline constexpr ui32 rotateLeft(ui32 val, ui32 rot) {
    return (val << rot) | (val >> ((sizeof(val) * 8) - rot));
}

inline constexpr ui64 rotateLeft(ui64 val, ui64 rot) {
    return (val << rot) | (val >> ((sizeof(val) * 8) - rot));
}

inline constexpr ui8 rotateRight(ui8 val, ui8 rot) {
    return (val >> rot) | (val << ((sizeof(val) * 8) - rot));
}

inline constexpr ui16 rotateRight(ui16 val, ui16 rot) {
    return (val >> rot) | (val << ((sizeof(val) * 8) - rot));
}

inline constexpr ui32 rotateRight(ui32 val, ui32 rot) {
    return (val >> rot) | (val << ((sizeof(val) * 8) - rot));
}

inline constexpr ui64 rotateRight(ui64 val, ui64 rot) {
    return (val >> rot) | (val << ((sizeof(val) * 8) - rot));
}

inline i8 bitScanF(ui64 n) {
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, n);
    return static_cast<ui8>(idx);
#elif defined(__GNUC__)
    return static_cast<i8>(__builtin_ctzll(n));
#else
#error No bitscan function
#endif
}

inline i8 bitScanR(ui64 n) {
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanReverse64(&idx, n);
    return static_cast<ui8>(idx);
#elif defined(__GNUC__)
    return static_cast<i8>(63 ^ __builtin_clzll(n));
#else
#error No bitscan function
#endif
}

} // lunachess

#endif // LUNA_BITS_H