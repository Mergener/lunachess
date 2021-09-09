#ifndef LUNA_TYPES_H
#define LUNA_TYPES_H

#include <cstdint>
#include <iostream>

#include "defs.h"

namespace lunachess {

typedef std::uint64_t ui64;
typedef std::uint32_t ui32;
typedef std::uint16_t ui16;
typedef std::uint8_t  ui8;

typedef std::int64_t i64;
typedef std::int32_t i32;
typedef std::int16_t i16;
typedef std::int8_t  i8;

//
// Side
//

BITWISE_ENUM_CLASS(Side, ui8,

	None = 0,
	White = 1,
	Black = 2

);

#define FOREACH_SIDE(s) for (Side s = Side::White; static_cast<int>(s) <= 2; s = (static_cast<Side>(static_cast<int>(s) + 1)) )

/*
	Returns the opposite side of a specified side.
*/
inline constexpr Side getOppositeSide(Side side) {
	return static_cast<Side>(static_cast<ui8>(side) ^ 3);
}

inline std::ostream& operator<<(std::ostream& stream, Side side) {
	switch (side) {
	case Side::White:
		stream << "White";
		break;

	case Side::Black:
		stream << "Black";
		break;

	default:
		stream << "None";
		break;
	}

	return stream;
}

//
// Lateral side
//

enum class LateralSide {
	KingSide,
	QueenSide
};

//
// Castling rights mask
//

BITWISE_ENUM_CLASS(CastlingRightsMask, ui8,
	None,
	WhiteOO = (1 << ((static_cast<int>(Side::White) - 1) * 2 + static_cast<int>(LateralSide::KingSide))),
	WhiteOOO = (1 << ((static_cast<int>(Side::White) - 1) * 2 + static_cast<int>(LateralSide::QueenSide))),
	BlackOO = (1 << ((static_cast<int>(Side::Black) - 1) * 2 + static_cast<int>(LateralSide::KingSide))),
	BlackOOO = (1 << ((static_cast<int>(Side::Black) - 1) * 2 + static_cast<int>(LateralSide::QueenSide))),
	All = WhiteOO | WhiteOOO | BlackOO | BlackOOO
);

}

#endif // LUNA_TYPES_H