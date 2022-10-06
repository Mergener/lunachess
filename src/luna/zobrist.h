#ifndef LUNA_ZOBRIST_H
#define LUNA_ZOBRIST_H

#include "piece.h"
#include "types.h"

namespace lunachess::zobrist {

void initialize();

inline ui64 getPieceSquareKey(Piece piece, Square sqr) {
    extern ui64 g_PieceSquareKeys[PT_COUNT][CL_COUNT][64];
    return g_PieceSquareKeys[piece.getType()][piece.getColor()][sqr];
}

inline ui64 getCastlingRightsKey(CastlingRightsMask crm) {
    extern ui64 g_CastlingRightsKey[16];
    return g_CastlingRightsKey[crm];
}

inline ui64 getColorToMoveKey(Color c) {
    extern ui64 g_ColorToMoveKey[2];
    return g_ColorToMoveKey[c];
}

inline ui64 getEnPassantSquareKey(Square sqr) {
    extern ui64 g_EnPassantSquareKey[256];
    return g_EnPassantSquareKey[sqr];
}

} // lunachess

#endif // LUNA_ZOBRIST_H
