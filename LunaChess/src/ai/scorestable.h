#ifndef LUNA_SCORESTABLE_H
#define LUNA_SCORESTABLE_H

#include <array>
#include <cstring>
#include <ostream>
#include <istream>

#include "../core/square.h"
#include "../core/piece.h"

//#include "json.hpp"

namespace lunachess::ai {

using PieceSquareTable = std::array<int, 64>;

extern PieceSquareTable g_OpPawnTable;
extern PieceSquareTable g_OpKnightTable;
extern PieceSquareTable g_OpBishopTable;
extern PieceSquareTable g_OpKingTable;
extern PieceSquareTable g_OpRookTable;
extern PieceSquareTable g_OpQueenTable;

extern PieceSquareTable g_EndPawnTable;
extern PieceSquareTable g_EndKnightTable;
extern PieceSquareTable g_EndBishopTable;
extern PieceSquareTable g_EndKingTable;
extern PieceSquareTable g_EndRookTable;
extern PieceSquareTable g_EndQueenTable;

struct ScoresTable {
	/** Score for each pawn that is in front of another pawn of same color in a file. */
	int blockingPawnsScore = 0;

	/** Score for each castling right available (king side/queen side) */
	int castlingRightsScore = 0;

	/** Score for each pawn that doesn't have an opposite-colored pawn further down their file. */
	int passedPawnScore = 0;
	/** Score for each pawn on a square of the opposite color of one of our bishops. */
	int goodColorPawnScore = 0;

	/** Score for each knight placed on a square that could never be attacked by an opponent's pawn. */
	int knightOutpostScore = 0;

	int pawnShieldScore = 0;

	int pawnChainScore = 0;

	int spaceScore = 0;

	int bishopPairScore = 0;

	int semiOpenFileNearKingScore = 0;

	int openFileNearKingScore = 0;

	/**
	 *	Score given for the king's proximity to pawns.
	 */
	int pawnKingDistanceScore = 0;

	/** Score for each piece being x-rayed, by type. */
	std::array<int, (int)PieceType::_Count> xrayScores;

	/** Score for king tropism for each piece */
	std::array<int, (int)PieceType::_Count> tropismScores;

	PieceSquareTable pieceSquareTables[(int)PieceType::_Count];

	inline ScoresTable() {
		std::memset(pieceSquareTables, 0, sizeof(pieceSquareTables));
		std::memset(xrayScores.data(), 0, sizeof(xrayScores));
	}
};

/*
void to_json(nlohmann::json& j, const ScoresTable& s);
void from_json(const nlohmann::json& j, ScoresTable& s);*/

}

#endif // LUNA_SCORESTABLE_H