#ifndef AIBITBOARDS_H
#define AIBITBOARDS_H

#include "../core/bitboard.h"
#include "../core/types.h"

namespace lunachess::ai::aibitboards {

void initialize();

Bitboard getKnightOPContestantsBitboard(Square s, Side side);
Bitboard getPawnShieldBitboard(Square kingSquare, Side side);

}

#endif // AIBITBOARDS_H