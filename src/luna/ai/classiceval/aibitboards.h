#ifndef LUNA_AI_AIBITBOARDS_H
#define LUNA_AI_AIBITBOARDS_H

#include "../../bitboard.h"

namespace lunachess::ai {

extern Bitboard g_FileContestantsBBs[64][CL_COUNT];
extern Bitboard g_PawnShields[64][CL_COUNT];
extern Bitboard g_PasserBlockers[64][2];
extern Bitboard g_NearKingSquares[64];

/**
 * Returns a bitboard of all the squares that can contain an opposing pawn
 * which could be pushed to attack the square 's'.
 */
inline Bitboard getFileContestantsBitboard(Square s, Color c) {
    return g_FileContestantsBBs[s][c];
}

inline Bitboard getPawnShieldBitboard(Square s, Color c) {
    return g_PawnShields[s][c];
}

inline Bitboard getPasserBlockerBitboard(Square s, Color c) {
    return g_PasserBlockers[s][c];
}

inline Bitboard getNearKingSquares(Square s) {
    return g_NearKingSquares[s];
}

void initializeAIBitboards();

}

#endif // LUNA_AI_AIBITBOARDS_H
