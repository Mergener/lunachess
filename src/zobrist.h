#ifndef LUNA_ZOBRIST_H
#define LUNA_ZOBRIST_H

#include "piece.h"
#include "types.h"

namespace lunachess::zobrist {

extern ui64 g_PieceSquareKeys[PT_COUNT][CL_COUNT][64];
extern ui64 g_CastlingRightsKey[16];
extern ui64 g_ColorToMoveKey[2];
extern ui64 g_EnPassantSquareKey[256];

void initialize();

inline ui64 getPieceSquareKey(Piece piece, Square sqr) {
    return g_PieceSquareKeys[piece.getType()][piece.getColor()][sqr];
}

inline ui64 getCastlingRightsKey(CastlingRightsMask crm) {
    return g_CastlingRightsKey[crm];
}

inline ui64 getColorToMoveKey(Color c) {
    return g_ColorToMoveKey[c];
}

inline ui64 getEnPassantSquareKey(Square sqr) {
    return g_EnPassantSquareKey[sqr];
}

}
#endif // LUNA_ZOBRIST_H
