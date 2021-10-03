#ifndef AIBITBOARDS_H
#define AIBITBOARDS_H

#include "../core/bitboard.h"
#include "../core/types.h"

namespace lunachess::ai::aibitboards {

/**
 * Returns a bitboard that contains all the squares an opponent must have a pawn
 * in order to prevent a knight outpost on a given square.
 * @param s The knight outpost square.
 * @param side The side of the knight.
 * @return The contestants bitboard.
 */
Bitboard getKnightOPContestantsBitboard(Square s, Side side);

/**
 * For a king on a given square, returns a bitboard containing all the squares for pawns that
 * can be considered shields for the king.
 * @param kingSquare The king square.
 * @param side The
 * @return The pawn shield bitboard.
 */
Bitboard getPawnShieldBitboard(Square kingSquare, Side side);

enum class FileState{
	Closed,
	SemiOpen,
	Open
};

FileState getFileState(int file, Bitboard whitePawns, Bitboard blackPawns);

struct PawnStructureAnalysis {
	Bitboard passedPawns    = 0;
	Bitboard backwardPawns  = 0;
	Bitboard connectedPawns = 0;
};

PawnStructureAnalysis analyzePawnStructure(Bitboard whitePawns, Bitboard blackPawns, Side side);

void initialize();

}

#endif // AIBITBOARDS_H