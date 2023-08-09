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

inline constexpr BoardRank getPromotionRank(Color c) {
    constexpr BoardRank PROM_RANKS[] = { PAWN_PROMOTION_RANK<CL_WHITE>, PAWN_PROMOTION_RANK<CL_BLACK> };
    return PROM_RANKS[c];
}

template <Color C>
constexpr BoardRank PAWN_INITIAL_RANK = C == CL_WHITE ? RANK_2 : RANK_7;

inline constexpr BoardRank getPawnInitialRank(Color c) {
    constexpr BoardRank INIT_RANKS[] = { PAWN_INITIAL_RANK<CL_WHITE>, PAWN_INITIAL_RANK<CL_BLACK> };
    return INIT_RANKS[c];
}

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

enum KingsDistribution {

    KD_KK, // Same side, both king side
    KD_KQ, // Opposite side, color A on king side
    KD_QK, // Opposite side, color A on queen side
    KD_QQ  // Same side, both queen side

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

void initializeDistances();

inline constexpr BoardFile getFile(Square s) {
	return static_cast<BoardFile>(s % 8);
}

inline constexpr BoardRank getRank(Square s) {
	return static_cast<BoardRank>(s / 8);
}

inline constexpr Square getSquare(BoardFile file, BoardRank rank) {
	return static_cast<Square>(static_cast<int>(rank * 8) + static_cast<int>(file));
}

inline constexpr Square getPromotionSquare(Color c, BoardFile f) {
    return getSquare(f, getPromotionRank(c));
}

inline constexpr Square mirrorHorizontally(Square s) {
    constexpr Square MIRRORS[] {
        SQ_H1, SQ_G1, SQ_F1, SQ_E1, SQ_D1, SQ_C1, SQ_B1, SQ_A1,
        SQ_H2, SQ_G2, SQ_F2, SQ_E2, SQ_D2, SQ_C2, SQ_B2, SQ_A2,
        SQ_H3, SQ_G3, SQ_F3, SQ_E3, SQ_D3, SQ_C3, SQ_B3, SQ_A3,
        SQ_H4, SQ_G4, SQ_F4, SQ_E4, SQ_D4, SQ_C4, SQ_B4, SQ_A4,
        SQ_H5, SQ_G5, SQ_F5, SQ_E5, SQ_D5, SQ_C5, SQ_B5, SQ_A5,
        SQ_H6, SQ_G6, SQ_F6, SQ_E6, SQ_D6, SQ_C6, SQ_B6, SQ_A6,
        SQ_H7, SQ_G7, SQ_F7, SQ_E7, SQ_D7, SQ_C7, SQ_B7, SQ_A7,
        SQ_H8, SQ_G8, SQ_F8, SQ_E8, SQ_D8, SQ_C8, SQ_B8, SQ_A8,
    };

    return MIRRORS[s];
}

inline constexpr Square mirrorVertically(Square s) {
    constexpr Square MIRRORS[] {
        SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
        SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
        SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
        SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
        SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
        SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
        SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
        SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    };

    return MIRRORS[s];
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
    extern int g_ChebyshevDistances[SQ_COUNT][SQ_COUNT];
    return g_ChebyshevDistances[a][b];
}

inline int getManhattanDistance(Square a, Square b) {
    extern int g_ManhattanDistances[SQ_COUNT][SQ_COUNT];
    return g_ManhattanDistances[a][b];
}

inline constexpr int getCenterManhattanDistance(Square s) {
    constexpr int CENTER_MANHATTAN_DISTANCE[] = {
        6, 5, 4, 3, 3, 4, 5, 6,
        5, 4, 3, 2, 2, 3, 4, 5,
        4, 3, 2, 1, 1, 2, 3, 4,
        3, 2, 1, 0, 0, 1, 2, 3,
        3, 2, 1, 0, 0, 1, 2, 3,
        4, 3, 2, 1, 1, 2, 3, 4,
        5, 4, 3, 2, 2, 3, 4, 5,
        6, 5, 4, 3, 3, 4, 5, 6
    };
    return CENTER_MANHATTAN_DISTANCE[s];
}

inline int stepsFromPromotion(Square s, Color c) {
    constexpr int STEPS_FROM_PROMOTION[2][64] { {
            0, 0, 0, 0, 0, 0, 0, 0,
            5, 5, 5, 5, 5, 5, 5, 5,
            5, 5, 5, 5, 5, 5, 5, 5,
            4, 4, 4, 4, 4, 4, 4, 4,
            3, 3, 3, 3, 3, 3, 3, 3,
            2, 2, 2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1, 1, 1,
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2, 2, 2,
            3, 3, 3, 3, 3, 3, 3, 3,
            4, 4, 4, 4, 4, 4, 4, 4,
            5, 5, 5, 5, 5, 5, 5, 5,
            5, 5, 5, 5, 5, 5, 5, 5,
            0, 0, 0, 0, 0, 0, 0, 0,
        }
    };
    return STEPS_FROM_PROMOTION[c][s];
}

const char* getSquareName(Square s);

//
// Time control types
//

enum TimeControlMode {
    TC_INFINITE,
    TC_TOURNAMENT,
    TC_MOVETIME
};

struct TimeControl {
    int time = 0;
    int increment = 0;
    TimeControlMode mode = TC_INFINITE;

	inline constexpr TimeControl() = default;
	inline TimeControl(const TimeControl& other) = default;
	inline TimeControl(TimeControl&& other) = default;
	inline TimeControl& operator=(const TimeControl& other) = default;

	inline constexpr TimeControl(int time, int increment, TimeControlMode mode)
		: time(time), increment(increment), mode(mode) {}
};

//
// Result types
//

enum ChessResult {
    RES_UNFINISHED,
    RES_DRAW_STALEMATE,
    RES_DRAW_REPETITION,
    RES_DRAW_TIME_NOMAT,
    RES_DRAW_NOMAT,
    RES_DRAW_RULE50,
    RES_WIN_CHECKMATE,
    RES_WIN_TIME,
    RES_WIN_RESIGN,
    RES_LOSS_CHECKMATE,
    RES_LOSS_TIME,
    RES_LOSS_RESIGN,

    //
    // Constants below create closed intervals for wins/draws/loss.
    // Use them to check for simple results.
    // Ex.:
    // bool isWin(const Position& pos, Color c, i64 remainingTime) {
    //     ChessResult res = pos.getResultForWhite(c, remainingTime > 0);
    //     return res >= RES_WIN_BEGIN && res <= RES_WIN_END;
    // }
    //

    RES_DRAW_BEGIN = RES_DRAW_STALEMATE,
    RES_DRAW_END = RES_DRAW_RULE50,
    RES_WIN_BEGIN = RES_WIN_CHECKMATE,
    RES_WIN_END = RES_WIN_RESIGN,
    RES_LOSS_BEGIN = RES_LOSS_CHECKMATE,
    RES_LOSS_END = RES_LOSS_RESIGN
};

inline constexpr bool isWin(ChessResult r) {
    return r >= RES_WIN_BEGIN && r <= RES_WIN_END;
}

inline constexpr bool isLoss(ChessResult r) {
    return r >= RES_LOSS_BEGIN && r <= RES_LOSS_END;
}

inline constexpr bool isDraw(ChessResult r) {
    return r >= RES_DRAW_BEGIN && r <= RES_DRAW_END;
}
/**
 * Turns a win result into the equivalent loss result or vice-versa and
 * returns it. Draw/Unfinished results are returned with no change.
 */
inline constexpr ChessResult getOppositeResult(ChessResult r) {
    switch (r) {
        case RES_WIN_CHECKMATE:  return RES_LOSS_CHECKMATE;
        case RES_WIN_TIME:       return RES_LOSS_TIME;
        case RES_WIN_RESIGN:     return RES_LOSS_RESIGN;
        case RES_LOSS_CHECKMATE: return RES_WIN_CHECKMATE;
        case RES_LOSS_TIME:      return RES_WIN_TIME;
        case RES_LOSS_RESIGN:    return RES_WIN_RESIGN;
        default:                 return r;
    }
}

} // lunachess

#endif // LUNA_TYPES_H
