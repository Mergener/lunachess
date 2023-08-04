#include "endgame.h"

#include <cstring>

#include "types.h"
#include "bitboard.h"

namespace lunachess::endgame {

static constexpr int MAX_PIECES = 11;

static EndgameData s_Endgames[BIT(8)][BIT(8)];

static constexpr ui64 buildEgMask(int nPawns, int nKnights,
                                  int nBishops, int nRooks,
                                  int nQueens) {
    ui64 mask = 0;

    mask |= nPawns    & BITMASK(3);
    mask |= (nBishops & BITMASK(2)) << 3;
    mask |= (nKnights & BITMASK(1)) << 5;
    mask |= (nRooks   & BITMASK(1)) << 6;
    mask |= (nQueens  & BITMASK(1)) << 7;

    return mask;
}

static void registerEndgame(EndgameType type, ui64 lhsMask, ui64 rhsMask) {
    s_Endgames[lhsMask][rhsMask] = EndgameData { type, CL_WHITE };
    s_Endgames[rhsMask][lhsMask] = EndgameData { type, CL_BLACK };
}

void initialize() {
    std::memset(s_Endgames, 0, sizeof(s_Endgames));

    registerEndgame(EG_KP_K, buildEgMask(1, 0, 0, 0, 0), 0);
    registerEndgame(EG_KR_K, buildEgMask(0, 0, 0, 1, 0), 0);
    registerEndgame(EG_KQ_K, buildEgMask(0, 0, 0, 0, 1), 0);
    registerEndgame(EG_KBB_K, buildEgMask(0, 0, 2, 0, 0), 0);
    registerEndgame(EG_KBP_K, buildEgMask(1, 0, 1, 0, 0), 0);
    registerEndgame(EG_KBN_K, buildEgMask(0, 1, 1, 0, 0), 0);

    registerEndgame(EG_KR_KN,
                    buildEgMask(0, 0, 0, 1, 0),
                    buildEgMask(0, 1, 0, 0, 0)
                    );

    registerEndgame(EG_KR_KB,
                    buildEgMask(0, 0, 0, 1, 0),
                    buildEgMask(0, 0, 1, 0, 0)
                    );

    registerEndgame(EG_KR_KR,
                    buildEgMask(0, 0, 0, 1, 0),
                    buildEgMask(0, 0, 0, 1, 0)
                    );

    registerEndgame(EG_KR_KR,
                    buildEgMask(0, 0, 0, 0, 1),
                    buildEgMask(0, 0, 0, 0, 1)
                    );
}

EndgameData identify(const Position& pos) {
    EndgameData ret;
    Bitboard occ = pos.getCompositeBitboard();
    int pieceCount = occ.count() - 2; // Discard kings
    if (pieceCount > MAX_PIECES) {
        return ret;
    }

    ui64 whiteMask = buildEgMask(pos.getBitboard(WHITE_PAWN).count(),
                               pos.getBitboard(WHITE_KNIGHT).count(),
                               pos.getBitboard(WHITE_BISHOP).count(),
                               pos.getBitboard(WHITE_ROOK).count(),
                               pos.getBitboard(WHITE_QUEEN).count());

    ui64 blackMask = buildEgMask(pos.getBitboard(BLACK_PAWN).count(),
                               pos.getBitboard(BLACK_KNIGHT).count(),
                               pos.getBitboard(BLACK_BISHOP).count(),
                               pos.getBitboard(BLACK_ROOK).count(),
                               pos.getBitboard(BLACK_QUEEN).count());


    EndgameData data = s_Endgames[whiteMask][blackMask];

    return data;
}

bool isInsideTheSquare(Square pawnSquare, Square enemyKingSquare,
                       Color pawnColor, Color colorToMove) {
    Square promSquare = getPromotionSquare(pawnColor, getFile(pawnSquare));
    int tempoPenalty = colorToMove != pawnColor ? 1 : 0;
    return (std::min(5, getChebyshevDistance(pawnSquare, promSquare)) >=
           getChebyshevDistance(enemyKingSquare, promSquare) - tempoPenalty);
}

}