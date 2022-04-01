#ifndef LUNA_AI_AIBITBOARDS_H
#define LUNA_AI_AIBITBOARDS_H

#include "../bitboard.h"

namespace lunachess::ai {

/**
 * Returns a bitboard that contains all the squares an opponent must have a pawn
 * in order to prevent a knight outpost on a given square.
 * @param s The knight outpost square.
 * @param c The color of the knight.
 * @return The contestants bitboard.
 */
Bitboard getFileContestantsBitboard(Square s, Color c);

Bitboard getPawnShieldBitboard(Square s, Color c);

Bitboard getPasserBlockerBitboard(Square s, Color c);

void initializeAIBitboards();

}

#endif // LUNA_AI_AIBITBOARDS_H
