#ifndef LUNA_BITS_H
#define LUNA_BITS_H

#include "defs.h"
#include "types.h"

#define HAS_BITSCAN_INTRINSICS
#ifdef HAS_BITSCAN_INTRINSICS
	#ifdef __GNUC__
		#include <cpuid.h>
	#elif _MSC_VER
		#include <intrin.h>
		#pragma intrinsic(_BitScanForward64)
		#pragma intrinsic(_BitScanReverse64)
	#endif
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

inline bool bitScanF(ui64 n, i8& bit) {
#ifdef HAS_BITSCAN_INTRINSICS
	unsigned long idx;
	unsigned char found = _BitScanForward64(&idx, n);

	bit = static_cast<ui8>(idx);
	return found;
#else
	for (int i = 0; i < 64; ++i) {
		if ((n & (C64(1) << i)) != 0) {
			bit = i;
			return true;
		}
	}
	return false;
#endif
}

inline bool bitScanR(ui64 n, i8& bit) {
#ifdef HAS_BITSCAN_INTRINSICS
	unsigned long idx;
	unsigned char found = _BitScanReverse64(&idx, n);

	bit = static_cast<ui8>(idx);
	return found;
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