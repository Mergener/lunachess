#include "aimovegen.h"

#include <algorithm>

#include "../movegen.h"
#include "hotmap.h"

namespace lunachess::ai {

static Hotmap s_MvOrderHotmaps[PT_COUNT] = {
    { // PT_NONE
        0,    0,   0,   0,   0,   0,   0,   0,
        0,    0,   0,   0,   0,   0,   0,   0,
        0,    0,   0,   0,   0,   0,   0,   0,
        0,    0,   0,   0,   0,   0,   0,   0,
        0,    0,   0,   0,   0,   0,   0,   0,
        0,    0,   0,   0,   0,   0,   0,   0,
        0,    0,   0,   0,   0,   0,   0,   0,
        0,    0,   0,   0,   0,   0,   0,   0,
    },
    { // PT_PAWN
        0,    0,   0,   0,   0,   0,   0,   0,
        98,  134, 61,  95,  68,  126, 34,  -11,
        -6,  7,   26, 31,  65,  56,  25, -20,
        -14, 13,  6,   21,  23,  12,  17,  -23,
        -27, -2,  -5,  12,  17,  6,   10,  -25,
        -26, -4,  -4,  -10, 3,   3,   33,  -12,
        -35, -1,  -20, -23, -15, 24,  38,  -22,
        0,    0,   0,   0,   0,   0,   0,   0,
    },
    { // PT_KNIGHT
        -167, -89, -34, -49, 61,  -97, -15, -107,
        -73, -41, 72,  36,  23,  62,  7,   -17,
        -47, 60,  37, 65,  84,  129, 73, 44,
        -9,  17,  19,  53,  37,  69,  18,  22,
        -13, 4,   16,  13,  28,  19,  21,  -8,
        -23, -9,  12,  10,  19,  17,  25,  -16,
        -29, -53, -12, -3,  -1,  18,  -14, -19,
        -105, -21, -58, -33, -17, -28, -19, -23,
    },
    { // PT_BISHOP
        -29,  4,   -82, -37, -25, -42, 7,   -8,
        -26, 16,  -18, -13, 30,  59,  18,  -47,
        -16, 37,  43, 40,  35,  50,  37, -2,
        -4,  5,   19,  50,  37,  37,  7,   -2,
        -6,  13,  13,  26,  34,  12,  10,  4,
        0,   15,  15,  15,  14,  27,  18,  10,
        4,   15,  16,  0,   7,   21,  33,  1,
        -33,  -3,  -14, -21, -13, -12, -39, -21,
    },
    { // PT_ROOK
        32,   42,  32,  51,  63,  9,   31,  43,
        27,  32,  58,  62,  80,  67,  26,  44,
        -5,  19,  26, 36,  17,  45,  61, 16,
        -24, -11, 7,   26,  24,  35,  -8,  -20,
        -36, -26, -12, -1,  9,   -7,  6,   -23,
        -45, -25, -16, -17, 3,   0,   -5,  -33,
        -44, -16, -20, -9,  -1,  11,  -6,  -71,
        -19,  -13, 1,   17,  16,  7,   -37, -26,
    },
    { // PT_QUEEN
        -28,  0,   29,  12,  59,  44,  43,  45,
        -24, -39, -5,  1,   -16, 57,  28,  54,
        -13, -17, 7,  8,   29,  56,  47, 57,
        -27, -27, -16, -16, -1,  17,  -2,  1,
        -9,  -26, -9,  -10, -2,  -4,  3,   -3,
        -14, 2,   -11, -2,  -5,  2,   14,  5,
        -35, -8,  11,  2,   8,   15,  -3,  1,
        -1,   -18, -9,  10,  -15, -25, -31, -50,
    },
    { // PT_KING
        -65,  23,  16,  -15, -56, -34, 2,   13,
        29,  -1,  -20, -7,  -8,  -4,  -38, -29,
        -9,  24,  2,  -16, -20, 6,   22, -22,
        -17, -20, -12, -27, -30, -25, -14, -36,
        -49, -1,  -27, -39, -46, -44, -33, -51,
        -14, -14, -22, -46, -44, -30, -15, -27,
        1,   7,   -8,  -64, -43, -16, 9,   8,
        -15,  36,  12,  -54, 8,   -28, 24,  14,
    }
};

int AIMoveFactory::getHotmapDelta(Move move) {
    Piece srcPiece = move.getSourcePiece();
    Color us = srcPiece.getColor();

    //const Hotmap& hotmap = s_MvOrderHotmaps[srcPiece.getType()];

    const Hotmap& hotmap = srcPiece.getType() == PT_KING
            ? Hotmap::defaultKingMgHotmap
            : Hotmap::defaultMiddlegameMaps[srcPiece.getType()].getHotmap(SQ_E1);

    int srcVal = hotmap.getValue(move.getSource(), us);
    int dstVal = hotmap.getValue(move.getDest(), us);
    int ret = dstVal - srcVal;

    return ret;
}

int AIMoveFactory::generateMoves(MoveList &ml, Position &pos, int currPly, Move pvMove) {
    int count = ml.size();

    // Generate noisy moves
    generateNoisyMoves(ml, pos, currPly, pvMove);

    // Generate remaining moves
    int noisyCount = ml.size() - count;
    movegen::generate<MTM_QUIET>(pos, ml);
    sortMoves<false>(pos, currPly, pvMove, ml.begin() + noisyCount, ml.end());

    return ml.size() - count;
}

int AIMoveFactory::generateNoisyMoves(MoveList &ml, Position &pos, int currPly, Move pvMove) {
    int count = ml.size();

    movegen::generate<MTM_NOISY>(pos, ml);

    // Moves generated, look for pv move
    MoveList::Iterator begin = ml.begin() + count;

    sortMoves<true>(pos, currPly, pvMove, begin, ml.end());

    return ml.size() - count;
}

void AIMoveFactory::storeKillerMove(Move move, int ply) {
    m_Killers[ply][1] = m_Killers[ply][0];
    m_Killers[ply][0] = move;
}

}