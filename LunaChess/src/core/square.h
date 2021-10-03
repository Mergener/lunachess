#ifndef LUNA_SQUARE_H
#define LUNA_SQUARE_H

#include <string_view>

#include "types.h"

namespace lunachess{

typedef i8 Square;

enum {

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

    SQ_INVALID = SQ_H8 + 1

};

namespace squares {

void initialize();

inline bool isValid(Square sq) {
    return (sq >= 0) && (sq < 64);
}

inline Square fromPoint(int x, int y) {
    return y * 8 + x;
}

inline int fileOf(Square sq) {
    return sq % 8;
}

inline int rankOf(Square sq) {
    return sq / 8;
}

const char* getName(Square sq);

inline int getPawnInitialRank(Side side) {
    static int rank[] = { 0, 1, 6 };

    return rank[(int)side];
}

/**
 *	Reads a chess square in algebraic notation (ex. d3, e1, a4)
 *
 * @param s The string to parse the square.
 * @return The square or SQ_INVALID.
 */
Square fromStr(std::string_view s);

Square getCastleRookSquare(Side side, LateralSide lateralSide);
Square getCastleRookDestSquare(Side side, LateralSide lateralSide);
Square getKingDefaultSquare(Side side);
Square getCastleKingDestSquare(Side side, LateralSide lateralSide);

int getChebyshevDistance(Square a, Square b);
int getManhattanDistance(Square a, Square b);
int getManhattanCenterDistance(Square s);

}

}

#endif // LUNA_SQUARE_H