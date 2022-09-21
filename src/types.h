#ifndef LUNA_TYPES_H
#define LUNA_TYPES_H

#include <iostream>
#include <cstdint>
#include <algorithm>

#include "debug.h"

#define C64(n) n##ULL

namespace lunachess {

//
//  Integer types
//

typedef std::int8_t    i8;
typedef std::int16_t   i16;
typedef std::int32_t   i32;
typedef std::int64_t   i64;
typedef std::uint8_t   ui8;
typedef std::uint16_t  ui16;
typedef std::uint32_t  ui32;
typedef std::uint64_t  ui64;

//
//  Chess basic types
//

typedef i8 Square;
typedef i8 PieceType;
typedef i8 Color;
typedef i8 Side;
typedef i8 Direction;
typedef i8 BoardFile;
typedef i8 BoardRank;

enum Colors {

	CL_WHITE,
	CL_BLACK,

	CL_COUNT = 2

};

inline constexpr Color getOppositeColor(Color c) { return c ^ 1; }
const char* getColorName(Color c);

enum Sides {

	SIDE_KING,
	SIDE_QUEEN,

	SIDE_COUNT = 2

};

const char* getSideName(Side s);

enum PieceTypes {

	PT_NONE,
	PT_PAWN,
	PT_KNIGHT,
	PT_BISHOP,
	PT_ROOK,
	PT_QUEEN,
	PT_KING,

	PT_COUNT = 7

};

inline constexpr int getPiecePointValue(PieceType pt) {
    constexpr int PT_VALUES[PT_COUNT] {
        0, 1, 3, 3, 5, 9, 99999
    };
    return PT_VALUES[pt];
}

const char* getPieceTypeName(PieceType pt);

enum Directions {

	DIR_NORTH = 8,
	DIR_SOUTH = -8,
	DIR_EAST = 1,
	DIR_WEST = -1,
	DIR_NORTHWEST = 7,
	DIR_NORTHEAST = 9,
	DIR_SOUTHWEST = -9,
	DIR_SOUTHEAST = -7,

	DIR_COUNT = 8

};

// Pawn move directions

template <Color C>
constexpr Direction PAWN_STEP_DIR = C == CL_WHITE ? DIR_NORTH : DIR_SOUTH;

inline constexpr Direction getPawnStepDir(Color c) {
	constexpr Direction STEP_DIRS[] {
		PAWN_STEP_DIR<CL_WHITE>,
        PAWN_STEP_DIR<CL_BLACK>
    };

	return STEP_DIRS[c];
}

template <Color C>
constexpr Direction PAWN_CAPT_LEFT_DIR = C == CL_WHITE ? DIR_NORTHWEST : DIR_SOUTHWEST;

template <Color C>
constexpr Direction PAWN_CAPT_RIGHT_DIR = C == CL_WHITE ? DIR_NORTHEAST : DIR_SOUTHEAST;

inline constexpr Direction getPawnLeftCaptureDir(Color c) {
    constexpr Direction CAPT_DIR[CL_COUNT] {
        PAWN_CAPT_LEFT_DIR<CL_WHITE>,
        PAWN_CAPT_LEFT_DIR<CL_BLACK>
    };

    return CAPT_DIR[c];
}

inline constexpr Direction getPawnRightCaptureDir(Color c) {
    constexpr Direction CAPT_DIR[CL_COUNT] {
            PAWN_CAPT_RIGHT_DIR<CL_WHITE>,
            PAWN_CAPT_RIGHT_DIR<CL_BLACK>
    };

    return CAPT_DIR[c];
}

const char* getDirectionName(Direction d);

enum BoardFiles {

	FL_A,
	FL_B,
	FL_C,
	FL_D,
	FL_E,
	FL_F,
	FL_G,
	FL_H,

	FL_COUNT = 8

};

inline char getFileIdentifier(BoardFile f) {
	if (f >= FL_COUNT || f < FL_A) {
		return '?';
	}
	constexpr char IDS[] { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };
	return IDS[f];
}

enum BoardRanks {

	RANK_1,
	RANK_2,
	RANK_3,
	RANK_4,
	RANK_5,
	RANK_6,
	RANK_7,
	RANK_8,

	RANK_COUNT = 8

};

template <Color C>
constexpr BoardRank PAWN_PROMOTION_RANK = C == CL_WHITE ? RANK_8 : RANK_1;

template <Color C>
constexpr BoardRank PAWN_INITIAL_RANK = C == CL_WHITE ? RANK_2 : RANK_7;

inline char getRankIdentifier(BoardRank r) {
	if (r >= RANK_COUNT || r < RANK_1) {
		return '?';
	}
	constexpr char IDS[] { '1', '2', '3', '4', '5', '6', '7', '8' };
	return IDS[r];
}

enum CastlingRightsMask {

	CR_NONE = 0,
	CR_WHITE_OO = (1 << (static_cast<int>(CL_WHITE) * 2 + static_cast<int>(SIDE_KING))),
	CR_WHITE_OOO = (1 << (static_cast<int>(CL_WHITE) * 2 + static_cast<int>(SIDE_QUEEN))),
	CR_BLACK_OO = (1 << (static_cast<int>(CL_BLACK) * 2 + static_cast<int>(SIDE_KING))),
	CR_BLACK_OOO = (1 << (static_cast<int>(CL_BLACK) * 2 + static_cast<int>(SIDE_QUEEN))),
	CR_ALL = CR_WHITE_OO | CR_WHITE_OOO | CR_BLACK_OO | CR_BLACK_OOO

};

enum Squares {

	SQ_A1 = 00,
	SQ_B1 = 01,
	SQ_C1 = 02,
	SQ_D1 = 03,
	SQ_E1 = 04,
	SQ_F1 = 05,
	SQ_G1 = 06,
	SQ_H1 = 07,

	SQ_A2 = 010,
	SQ_B2 = 011,
	SQ_C2 = 012,
	SQ_D2 = 013,
	SQ_E2 = 014,
	SQ_F2 = 015,
	SQ_G2 = 016,
	SQ_H2 = 017,

	SQ_A3 = 020,
	SQ_B3 = 021,
	SQ_C3 = 022,
	SQ_D3 = 023,
	SQ_E3 = 024,
	SQ_F3 = 025,
	SQ_G3 = 026,
	SQ_H3 = 027,

	SQ_A4 = 030,
	SQ_B4 = 031,
	SQ_C4 = 032,
	SQ_D4 = 033,
	SQ_E4 = 034,
	SQ_F4 = 035,
	SQ_G4 = 036,
	SQ_H4 = 037,

	SQ_A5 = 040,
	SQ_B5 = 041,
	SQ_C5 = 042,
	SQ_D5 = 043,
	SQ_E5 = 044,
	SQ_F5 = 045,
	SQ_G5 = 046,
	SQ_H5 = 047,

	SQ_A6 = 050,
	SQ_B6 = 051,
	SQ_C6 = 052,
	SQ_D6 = 053,
	SQ_E6 = 054,
	SQ_F6 = 055,
	SQ_G6 = 056,
	SQ_H6 = 057,

	SQ_A7 = 060,
	SQ_B7 = 061,
	SQ_C7 = 062,
	SQ_D7 = 063,
	SQ_E7 = 064,
	SQ_F7 = 065,
	SQ_G7 = 066,
	SQ_H7 = 067,

	SQ_A8 = 070,
	SQ_B8 = 071,
	SQ_C8 = 072,
	SQ_D8 = 073,
	SQ_E8 = 074,
	SQ_F8 = 075,
	SQ_G8 = 076,
	SQ_H8 = 077,

	SQ_INVALID = SQ_H8 + 1,
	SQ_COUNT = 64

};

#define ASSERT_VALID_SQUARE(s) LUNA_ASSERT(static_cast<ui8>(s) >= 0, "Invalid square. (got " << int(s) << ")")

inline constexpr BoardFile getFile(Square s) {
	return static_cast<BoardFile>(s % 8);
}

inline constexpr BoardRank getRank(Square s) {
	return static_cast<BoardRank>(s / 8);
}

inline constexpr Square getSquare(BoardFile file, BoardRank rank) {
	return static_cast<Square>(static_cast<int>(rank * 8) + static_cast<int>(file));
}

Square getSquare(std::string_view str);

inline constexpr Square getCastleRookSrcSquare(Color color, Side side) {
    constexpr Square CASTLE_ROOK_SQ[CL_COUNT][SIDE_COUNT] = {
            { SQ_H1, SQ_A1 }, // White
            { SQ_H8, SQ_A8 }  // Black
    };

    return CASTLE_ROOK_SQ[color][side];
}

inline constexpr Square getCastleRookDestSquare(Color color, Side side) {
    constexpr Square CASTLE_ROOK_SQ[CL_COUNT][SIDE_COUNT] = {
            { SQ_F1, SQ_D1 }, // White
            { SQ_F8, SQ_D8 }  // Black
    };

    return CASTLE_ROOK_SQ[color][side];
}

inline int getChebyshevDistance(Square a, Square b) {
    int fileDist = std::abs(getFile(a) - getFile(b));
    int rankDist = std::abs(getRank(a) - getRank(b));
    return std::max(fileDist, rankDist);
}

inline int getManhattanDistance(Square a, Square b) {
    int fileDist = std::abs(getFile(a) - getFile(b));
    int rankDist = std::abs(getRank(a) - getRank(b));
    return fileDist + rankDist;
}

const char* getSquareName(Square s);

//
// Time control types
//

enum TimeControlMode {
    TC_INFINITE,
    TC_FISCHER,
    TC_MOVETIME
};

struct TimeControl {
    int time = 0;
    int increment = 0;
    TimeControlMode mode = TC_INFINITE;

	inline TimeControl() = default;
	inline TimeControl(const TimeControl& other) = default;
	inline TimeControl(TimeControl&& other) = default;
	inline TimeControl& operator=(const TimeControl& other) = default;
	inline ~TimeControl() = default;	

	inline TimeControl(int time, int increment, TimeControlMode mode)
		: time(time), increment(increment), mode(mode) {}
};

} // lunachess

#endif // LUNA_TYPES_H
