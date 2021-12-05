#include "basicevaluator.h"

#include <iostream>

#include "aibitboards.h"

#include "../core/position.h"

namespace lunachess::ai {

static int getPieceMaterialScore(PieceType pt) {
	switch (pt) {
	default: 
		return 0;

	case PieceType::Pawn:
		return 100;

	case PieceType::Knight:
		return 310;

	case PieceType::Bishop:
		return 340;

	case PieceType::Rook:
		return 510;

	case PieceType::Queen:
		return 910;

	case PieceType::King:
		return 20000;
	}
}

int BasicEvaluator::getPawnColorScore(const Position& pos, Side side, int gpp) const {
	// Score pawns that are on squares that favor the movement of a side's bishop.

	Bitboard lightSquares = bitboards::getLightSquares();
	Bitboard darkSquares = bitboards::getDarkSquares();

	Bitboard lightSquaredBishops = pos.getPieceBitboard(Piece(side, PieceType::Bishop)) & lightSquares;
	Bitboard darkSquaredBishops = pos.getPieceBitboard(Piece(side, PieceType::Bishop)) & darkSquares;

	Bitboard desiredColorComplex;
	int nlsb = lightSquaredBishops.count();
	int ndsb = darkSquaredBishops.count();
	if (nlsb == ndsb) {
		return 0;
	}
	if (ndsb > nlsb) {
		// More dark squared bishops, we want more pawns in light squares
		desiredColorComplex = lightSquares;
	}
	else {
		// More light squared bishops, we want more pawns in dark squares
		desiredColorComplex = darkSquares;
	}

	int individualScore = interpolateScores(m_OpScoresTable.goodColorPawnScore, m_EndScoresTable.goodColorPawnScore, gpp);

	Bitboard pawns = pos.getPieceBitboard(PieceType::Pawn, side) & desiredColorComplex;

	return pawns.count() * individualScore;
}

int BasicEvaluator::getBlockingPawnScore(const Position& pos, Side side, int gpp) const {
	int total = 0;
	Bitboard bb = pos.getPieceBitboard(PieceType::Pawn, side);

	int individualScore = interpolateScores(m_OpScoresTable.blockingPawnsScore, m_EndScoresTable.blockingPawnsScore, gpp);

	for (int i = 0; i < 8; ++i) {
		// For each file, do the check
		Bitboard fileBB = bitboards::getFileBitboard(i);

		Bitboard intersection = bb & fileBB;

		// If two or more pawns of same color are in a file,
		// they are blocking pawns.
		total += (intersection.count() - 1) * individualScore;
	}
	return total;
}

bool BasicEvaluator::isKnightOutpost(const Position& pos, Square sq, Side side) const {
	Bitboard opponentPawnBB = pos.getPieceBitboard(PieceType::Pawn, getOppositeSide(side));
	Bitboard contestantBB = aibitboards::getKnightOPContestantsBitboard(sq, side);
	contestantBB = contestantBB & opponentPawnBB;
	
	return contestantBB.count() == 0;
}

int BasicEvaluator::getKnightOutpostScore(const Position& pos, Side side, int gpp) const {
	int total = 0;
	int individualScore = interpolateScores(m_OpScoresTable.knightOutpostScore, m_EndScoresTable.knightOutpostScore, gpp);

	Bitboard knightBB = pos.getPieceBitboard(PieceType::Knight, side);
	Bitboard opponentPawnBB = pos.getPieceBitboard(PieceType::Pawn, getOppositeSide(side));

	for (auto sq : knightBB) {
		Bitboard contestantBB = aibitboards::getKnightOPContestantsBitboard(sq, side);

		// This knight is in an outpost if there are no opponent pawns within the contestant bitboard.
		contestantBB = contestantBB & opponentPawnBB;
		if (contestantBB.count() == 0) {
			total += individualScore;
		}
	}

	return total;
}

int BasicEvaluator::getKingSafetyScore(const Position& pos, Side side, int gpp) const {
	Square kingSquare = pos.getKingSquare(side);

	// Count shield pawns
	int individualPawnShieldScore = interpolateScores(m_OpScoresTable.pawnShieldScore, m_EndScoresTable.pawnShieldScore, gpp);
	Bitboard pawnShieldBB = aibitboards::getPawnShieldBitboard(kingSquare, side);
	Bitboard shieldPawns = pos.getPieceBitboard(PieceType::Pawn, side) & pawnShieldBB;

	// Tropism
	int tropismScore = 0;
	
	Side opponent = getOppositeSide(side);

    bool weHaveMoreMaterial = pos.getMaterialCount(side) > pos.getMaterialCount(opponent);

	FOREACH_PIECE_TYPE(pt) {
        if (pt == PieceType::King && !weHaveMoreMaterial) {
            // King is only good closer to the other king when we have more material.
            // This encourages the engine to chase the opponent's king with their king in endgames.
            continue;
        }

		// Get tropism score for each piece type
		int trop = interpolateScores(m_OpScoresTable.tropismScores[(int)pt], m_EndScoresTable.tropismScores[(int)pt], gpp);
		int sqrTrop = trop * trop;

		Bitboard bb = pos.getPieceBitboard(pt, opponent);
		for (auto s : bb) {
			// For every opponent piece, count tropism score based on their
			// distance from our king
			int kingX = squares::fileOf(kingSquare);
			int kingY = squares::rankOf(kingSquare);

			int sx = squares::fileOf(s);
			int sy = squares::rankOf(s);

			int pseudoDist = std::abs(sx - kingX) + std::abs(sy - kingY);
			if (pseudoDist == 0) {
				pseudoDist = 1;
			}
			tropismScore -= sqrTrop / pseudoDist; // The higher the distance, the lower the penalty.
		}
	}

	return shieldPawns.count() * individualPawnShieldScore + tropismScore;
}

int BasicEvaluator::getPawnChainScore(const Position& pos, Side side, int gpp) const {
	Bitboard pawnBB = pos.getPieceBitboard(PieceType::Pawn, side);

	bool prevHasPawn = (pawnBB & bitboards::getFileBitboard(0)) != 0;
	int individualScore = interpolateScores(m_OpScoresTable.pawnChainScore, m_EndScoresTable.pawnChainScore, gpp);
	int total = 0;

	for (int i = 1; i < 8; ++i) {
		bool thisHasPawn = (pawnBB & bitboards::getFileBitboard(i)) != 0;
		if (thisHasPawn && prevHasPawn) {
			total += individualScore;
		}
		prevHasPawn = thisHasPawn;
	}
	return total;
}

int BasicEvaluator::getBishopPairScore(const Position& pos, Side side, int gpp) const {
	Bitboard lightSquares = bitboards::getLightSquares();
	Bitboard darkSquares = bitboards::getDarkSquares();

	auto bishopBB = pos.getPieceBitboard(Piece(side, PieceType::Bishop));
	
	Bitboard lightSquaredBishops = bishopBB & lightSquares;
	Bitboard darkSquaredBishops = bishopBB & darkSquares;

	int individualScore = interpolateScores(m_OpScoresTable.bishopPairScore, m_EndScoresTable.bishopPairScore, gpp);

	return std::min(lightSquaredBishops.count(), darkSquaredBishops.count()) * individualScore;
}

int BasicEvaluator::evaluate(const Position& pos) const {
	int total = 0;
	Side side = pos.getSideToMove();

	int gpp = getGamePhaseFactor(pos);
	int individualSpaceScore = interpolateScores(
		m_OpScoresTable.spaceScore,
		m_EndScoresTable.spaceScore,
		gpp);

	for (Square s = 0; s < 64; ++s) {
		auto p = pos.getPieceAt(s);
		if (p == PIECE_NONE) {
			continue;
		}

		int sign = p.getSide() == side ? 1 : -1;
		
		total += getPieceMaterialScore(p.getType()) * sign;

		Square evalSqr;
		if (p.getSide() == Side::White) {
			int file = squares::fileOf(s);
			int rank = squares::rankOf(s);

			rank = 7 - rank;
			evalSqr = rank * 8 + file;
		}
		else {
			evalSqr = s;
		}		

		total += (sign * interpolateScores(
			m_OpScoresTable.pieceSquareTables[evalSqr][(int)p.getType()],
			m_EndScoresTable.pieceSquareTables[evalSqr][(int)p.getType()],
			gpp)) / 3;

		// See xrays
		auto attacks = bitboards::getPieceAttacks(p.getType(), 0, s, p.getSide());
		auto opponent = getOppositeSide(p.getSide());

		for (Square as : attacks) {
			auto op = pos.getPieceAt(as);
			if (op.getSide() != opponent) {
				continue;
			}
			int individualScore = interpolateScores(m_OpScoresTable.xrayScores[(int)op.getType()],
				m_EndScoresTable.xrayScores[(int)op.getType()], gpp) * sign;
			total += individualScore;
		}

		// Add space score
		if (p.getType() != PieceType::Queen) {
			total += sign * bitboards::getPieceAttacks(p.getType(), pos.getCompositeBitboard(), s, p.getSide()).count() * individualSpaceScore;
		}
	}

	Side oppositeSide = getOppositeSide(side);
	total += getBishopPairScore(pos, side, gpp) - getBishopPairScore(pos, oppositeSide, gpp);
	total += getBlockingPawnScore(pos, side, gpp) - getBlockingPawnScore(pos, oppositeSide, gpp);
	total += getPawnColorScore(pos, side, gpp) - getPawnColorScore(pos, oppositeSide, gpp);
	total += getKnightOutpostScore(pos, side, gpp) - getKnightOutpostScore(pos, oppositeSide, gpp);
	total += getKingSafetyScore(pos, side, gpp) - getKingSafetyScore(pos, oppositeSide, gpp);
	total += getPawnChainScore(pos, side, gpp) - getPawnChainScore(pos, oppositeSide, gpp);

	return total;
}

int BasicEvaluator::getDrawScore() const {
	return 0;
}

BasicEvaluator::BasicEvaluator() {

	m_OpScoresTable.pieceSquareTables[(int)PieceType::Pawn] = g_OpPawnTable;
	m_OpScoresTable.pieceSquareTables[(int)PieceType::Knight] = g_OpKnightTable;
	m_OpScoresTable.pieceSquareTables[(int)PieceType::Bishop] = g_OpBishopTable;
	m_OpScoresTable.pieceSquareTables[(int)PieceType::King] = g_OpKingTable;
	m_OpScoresTable.pieceSquareTables[(int)PieceType::Rook] = g_OpRookTable;
	m_OpScoresTable.pieceSquareTables[(int)PieceType::Queen] = g_OpQueenTable;
	m_OpScoresTable.blockingPawnsScore = -20;
	m_OpScoresTable.goodColorPawnScore = 5;
	m_OpScoresTable.xrayScores[(int)PieceType::Pawn] = 0;
	m_OpScoresTable.xrayScores[(int)PieceType::Bishop] = 2;
	m_OpScoresTable.xrayScores[(int)PieceType::Knight] = 2;
	m_OpScoresTable.xrayScores[(int)PieceType::Rook] = 3;
	m_OpScoresTable.xrayScores[(int)PieceType::Queen] = 4;
	m_OpScoresTable.xrayScores[(int)PieceType::King] = 5;
	m_OpScoresTable.bishopPairScore = 15;
	m_OpScoresTable.tropismScores[(int)PieceType::Pawn] = 12;
	m_OpScoresTable.tropismScores[(int)PieceType::Bishop] = 5;
	m_OpScoresTable.tropismScores[(int)PieceType::Knight] = 12;
	m_OpScoresTable.tropismScores[(int)PieceType::Rook] = 8;
	m_OpScoresTable.tropismScores[(int)PieceType::Queen] = 4;
	m_OpScoresTable.tropismScores[(int)PieceType::King] = 0;
	m_OpScoresTable.knightOutpostScore = 15;
	m_OpScoresTable.spaceScore = 3;
	m_OpScoresTable.pawnShieldScore = 12;
	m_OpScoresTable.pawnChainScore = 4;
	m_OpScoresTable.pawnKingDistanceScore = 0;
	m_OpScoresTable.openFileNearKingScore = -30;
	m_OpScoresTable.semiOpenFileNearKingScore = -20;

	// ENDGAME

	m_EndScoresTable.pieceSquareTables[(int)PieceType::Pawn] = g_EndPawnTable;
	m_EndScoresTable.pieceSquareTables[(int)PieceType::Knight] = g_EndKnightTable;
	m_EndScoresTable.pieceSquareTables[(int)PieceType::Bishop] = g_EndBishopTable;
	m_EndScoresTable.pieceSquareTables[(int)PieceType::King] = g_EndKingTable;
	m_EndScoresTable.pieceSquareTables[(int)PieceType::Rook] = g_EndRookTable;
	m_EndScoresTable.pieceSquareTables[(int)PieceType::Queen] = g_EndQueenTable;
	m_EndScoresTable.tropismScores[(int)PieceType::Pawn] = 0;
	m_EndScoresTable.tropismScores[(int)PieceType::Bishop] = 7;
	m_EndScoresTable.tropismScores[(int)PieceType::Knight] = 14;
	m_EndScoresTable.tropismScores[(int)PieceType::Rook] = 10;
	m_EndScoresTable.tropismScores[(int)PieceType::Queen] = 8;
	m_EndScoresTable.tropismScores[(int)PieceType::King] = 7;
	m_EndScoresTable.blockingPawnsScore = -60;
	m_EndScoresTable.goodColorPawnScore = 1;
	m_EndScoresTable.knightOutpostScore = 10;
	m_EndScoresTable.spaceScore = 1;
	m_EndScoresTable.pawnShieldScore = 0;
	m_EndScoresTable.pawnChainScore = 8;
	m_EndScoresTable.bishopPairScore = 40;
	m_EndScoresTable.pawnKingDistanceScore = 3;
	m_EndScoresTable.openFileNearKingScore = 0;
	m_EndScoresTable.semiOpenFileNearKingScore = 0;
}

int BasicEvaluator::getGamePhaseFactor(const Position& pos) const {
	constexpr int STARTING_MATERIAL_COUNT = 62; // Excluding pawns

	int totalMaterial = pos.countTotalPointValue(false);
	int ret = (totalMaterial * 100) / STARTING_MATERIAL_COUNT;

	return ret;
}

int BasicEvaluator::interpolateScores(int earlyScore, int lateScore, int gamePhaseFactor) const {
	return lateScore + ((earlyScore - lateScore) * gamePhaseFactor) / 100;
}

}