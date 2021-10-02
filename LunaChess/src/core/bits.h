#ifndef LUNA_BITS_H
#define LUNA_BITS_H

#include "defs.h"
#include "types.h"
#include "debug.h"

#ifdef __GNUC__
    #include <cpuid.h>
#elif _MSC_VER
    #include <intrin.h>
    #pragma intrinsic(_BitScanForward64)
    #pragma intrinsic(_BitScanReverse64)
#endif

namespace lunachess::bits {

inline ui8 rotateLeft(ui8 val, ui8 rot) {
	return (val << rot) | (val >> ((sizeof(val) * 8) - rot));
}

inline ui16 rotateLeft(ui16 val, ui16 rot) {
	return (val << rot) | (val >> ((sizeof(val) * 8) - rot));
}

inline ui32 rotateLeft(ui32 val, ui32 rot) {
	return (val << rot) | (val >> ((sizeof(val) * 8) - rot));
}

inline ui64 rotateLeft(ui64 val, ui64 rot) {
	return (val << rot) | (val >> ((sizeof(val) * 8) - rot));
}

inline ui8 rotateRight(ui8 val, ui8 rot) {
	return (val >> rot) | (val << ((sizeof(val) * 8) - rot));
}

inline ui16 rotateRight(ui16 val, ui16 rot) {
	return (val >> rot) | (val << ((sizeof(val) * 8) - rot));
}

inline ui32 rotateRight(ui32 val, ui32 rot) {
	return (val >> rot) | (val << ((sizeof(val) * 8) - rot));
}

inline ui64 rotateRight(ui64 val, ui64 rot) {
	return (val >> rot) | (val << ((sizeof(val) * 8) - rot));
}

inline i8 bitScanF(ui64 n) {
#if defined(__MSC_VER)
	unsigned long idx;
	_BitScanForward64(&idx, n);
	return static_cast<ui8>(idx);
#elif defined(__GNUC__)
    return static_cast<i8>(__builtin_ctzll(n));
#else
	for (int i = 0; i < 64; ++i) {
		if ((n & (C64(1) << i)) != 0) {
			return i;
		}
	}
	// Unreachable
	return 0;
#endif
}

inline i8 bitScanR(ui64 n) {
#if defined(__MSC_VER)
	unsigned long idx;
	_BitScanReverse64(&idx, n);
	return static_cast<ui8>(idx);
#elif defined(__GNUC__)
	return static_cast<i8>(63 ^ __builtin_clzll(n));
#else
	for (int i = 63; i >= 0; --i) {
		if ((n & (C64(1) << i)) != 0) {
			bit = i;
			return true;
		}
	}
	return false;
#endif
}

}

#endif // LUNA_BITS_H