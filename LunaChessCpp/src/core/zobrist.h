#ifndef LUNA_ZOBRIST_H
#define LUNA_ZOBRIST_H

#include "square.h"
#include "piece.h"
#include "types.h"

namespace lunachess::zobrist {

void initialize();
ui64 getPieceSquareKey(Piece piece, Square sqr);
ui64 getCastlingRightsKey(CastlingRightsMask crm);
ui64 getSideToMoveKey(Side side);
ui64 getEnPassantSquareKey(Square sqr);

}

#endif // LUNA_ZOBRIST_H