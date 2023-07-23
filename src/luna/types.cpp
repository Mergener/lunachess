#include "types.h"

namespace lunachess {

const char* getColorName(Color c) {
    switch (c) {
        case CL_WHITE:
            return "White";

        case CL_BLACK:
            return "Black";

        default:
            return "Unknown Color";
    }
}

const char* getSideName(Side s) {
    switch (s) {
        case SIDE_KING:
            return "King Side";

        case SIDE_QUEEN:
            return "Queen Side";

        default:
            return "Unknown Side";
    }
}

const char* getPieceTypeName(PieceType pt) {
    switch (pt) {
        case PT_PAWN:
            return "Pawn";

        case PT_KNIGHT:
            return "Knight";

        case PT_BISHOP:
            return "Bishop";

        case PT_ROOK:
            return "Rook";

        case PT_QUEEN:
            return "Queen";

        case PT_KING:
            return "King";

        case PT_NONE:
            return "No piece";

        default:
            return "Unknown Piece Type";
    }
}

const char* getDirectionName(Direction d) {
    switch (d) {
        case DIR_NORTH:
            return "North";

        case DIR_SOUTH:
            return "South";

        case DIR_EAST:
            return "East";

        case DIR_WEST:
            return "West";

        case DIR_NORTHEAST:
            return "Northeast";

        case DIR_NORTHWEST:
            return "Northwest";

        case DIR_SOUTHEAST:
            return "Southeast";

        case DIR_SOUTHWEST:
            return "Southwest";

        default:
            return "Unknown direction";
    }
}

int g_ChebyshevDistances[SQ_COUNT][SQ_COUNT];
int g_ManhattanDistances[SQ_COUNT][SQ_COUNT];

void initializeDistances() {
    for (Square a = 0; a < SQ_COUNT; ++a) {
        for (Square b = 0; b < SQ_COUNT; ++b) {
            int fileDist = std::abs(getFile(a) - getFile(b));
            int rankDist = std::abs(getRank(a) - getRank(b));
            g_ManhattanDistances[a][b] = fileDist + rankDist;
            g_ChebyshevDistances[a][b] = std::max(fileDist, rankDist);
        }
    }

}



const char* s_SquareNames[64] = {
        "a1", "b1", "c1", "d1",
        "e1", "f1", "g1", "h1",
        "a2", "b2", "c2", "d2",
        "e2", "f2", "g2", "h2",
        "a3", "b3", "c3", "d3",
        "e3", "f3", "g3", "h3",
        "a4", "b4", "c4", "d4",
        "e4", "f4", "g4", "h4",
        "a5", "b5", "c5", "d5",
        "e5", "f5", "g5", "h5",
        "a6", "b6", "c6", "d6",
        "e6", "f6", "g6", "h6",
        "a7", "b7", "c7", "d7",
        "e7", "f7", "g7", "h7",
        "a8", "b8", "c8", "d8",
        "e8", "f8", "g8", "h8"
};

const char* getSquareName(Square s) {
    if (s >= SQ_COUNT) {
        return "--";
    }

    return s_SquareNames[s];
}

Square getSquare(std::string_view str) {
    if (str.size() != 2) {
        return SQ_INVALID;
    }

    if (str[0] < 'a' || str[0] > 'h') {
        // Invalid file
        return SQ_INVALID;
    }

    if (str[1] < '1' || str[1] > '8') {
        // Invalid rank
        return SQ_INVALID;
    }

    BoardFile file = str[0] - 'a';
    BoardRank rank = str[1] - '1';

    return getSquare(file, rank);
}

}
