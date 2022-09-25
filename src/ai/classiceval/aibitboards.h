#ifndef LUNA_AI_AIBITBOARDS_H
#define LUNA_AI_AIBITBOARDS_H

#include "../../bitboard.h"

namespace lunachess::ai {

extern Bitboard g_FileContestantsBBs[64][CL_COUNT];
extern Bitboard g_PawnShields[64][CL_COUNT];
extern Bitboard g_PasserBlockers[64][2];
extern Bitboard g_NearKingSquares[64];

/**
 * Returns a bitboard that contains all the squares an opponent must have a pawn
 * in order to prevent a knight outpost on a given square.
 * @param s The knight outpost square.
 * @param c The color of the knight.
 * @return The contestants bitboard.
 */
inline Bitboard getFileContestantsBitboard(Square s, Side side) {
    return g_FileContestantsBBs[s][(int)side - 1];
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
