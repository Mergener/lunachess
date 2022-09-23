#include "endgame.h"

#include "types.h"
#include "bitboard.h"

namespace lunachess::endgame {

void initialize() {
}

static EndgameType get1ManEndgameType(const Position& pos, Color lhs) {
    // KP vs K
    if (pos.getBitboard(Piece(lhs, PT_PAWN)) != 0) {
        return EG_KP_K;
    }
    return EG_UNKNOWN;
}

static EndgameType getEndgameType(const Position &pos, int pieceCount, Color lhs) {
    switch (pieceCount) {
        case 1:  return get1ManEndgameType(pos, lhs);
        default: return EG_UNKNOWN;
    }
}

EndgameData identify(const Position& pos) {
    EndgameData ret;
    Bitboard occ = pos.getCompositeBitboard();
    int pieceCount = occ.count() - 2; // Discard kings

    EndgameType type = getEndgameType(pos, pieceCount, CL_WHITE);

    if (type == EG_UNKNOWN) {
        type = getEndgameType(pos, pieceCount, CL_BLACK);
        ret.lhs = CL_BLACK;
    }
    else {
        ret.lhs = CL_WHITE;
    }

    ret.type = type;
    return ret;
}

bool isInsideTheSquare(Square pawnSquare, Square enemyKingSquare,
                       Color pawnColor, Color colorToMove) {
    Square promSquare = getPromotionSquare(pawnColor, getFile(pawnSquare));
    int tempoPenalty = colorToMove != pawnColor ? 1 : 0;
    return (std::min(5, getChebyshevDistance(pawnSquare, promSquare)) >=
           getChebyshevDistance(enemyKingSquare, promSquare) - tempoPenalty);
}

}