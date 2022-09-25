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

static void initializeManhattanDistances() {
    for (Square a = 0; a < SQ_COUNT; ++a) {
        for (Square b = 0; b < SQ_COUNT; ++b) {
            int fileDist = std::abs(getFile(a) - getFile(b));
            int rankDist = std::abs(getRank(a) - getRank(b));
            g_ManhattanDistances[a][b] = fileDist + rankDist;
        }
    }
}
static void initializeChebyshevDistances() {
    for (Square a = 0; a < SQ_COUNT; ++a) {
        for (Square b = 0; b < SQ_COUNT; ++b) {
            int fileDist = std::abs(getFile(a) - getFile(b));
            int rankDist = std::abs(getRank(a) - getRank(b));
            g_ChebyshevDistances[a][b] = std::max(fileDist, rankDist);
        }
    }
}

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

}
