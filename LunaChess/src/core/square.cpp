#include "square.h"

#include "debug.h"

namespace lunachess::squares {

static const char* squareNames[] = {
	"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", 
	"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", 
	"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", 
	"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", 
	"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", 
	"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", 
	"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", 
	"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

const char* getName(Square square) {

#ifdef NDEBUG
	return squareNames[square];
#else
	if (square < 64 && square >= 0) {
		return squareNames[square];
	}
	else {
		return "SQ_INVALID";
	}
#endif
}

Square fromStr(std::string_view s) {
	int file = s[0] - 'a';
	if (file < 0 || file > 7) {
		// Try uppercase
		file = s[0] - 'A';
		if (file < 0 || file > 7) {
			return SQ_INVALID;
		}
	}

	int rank = s[1] - '1';
	if (rank < 0 || rank > 7) {
		return SQ_INVALID;
	}

	return rank * 8 + file;
}


static Square s_RookCastleSquares[2][2];
static Square s_RookCastleDestSquares[2][2];

Square getCastleRookSquare(Side side, LateralSide lateralSide) {
	return s_RookCastleSquares[(int)side - 1][(int)lateralSide];
}

Square getCastleRookDestSquare(Side side, LateralSide lateralSide) {
	return s_RookCastleDestSquares[(int)side - 1][(int)lateralSide];

}

static Square s_KingDefaultSquares[2];
static Square s_KingCastleSquares[2][2];

Square getCastleKingDestSquare(Side side, LateralSide lateralSide) {
	return s_KingCastleSquares[(int)side - 1][(int)lateralSide];
}

Square getKingDefaultSquare(Side side) {
	return s_KingDefaultSquares[(int)side - 1];
}

static void generateCastlingSquares() {
	// Rooks
	s_RookCastleSquares[(int)Side::White - 1][(int)LateralSide::KingSide] = SQ_H1;
	s_RookCastleSquares[(int)Side::White - 1][(int)LateralSide::QueenSide] = SQ_A1;

	s_RookCastleSquares[(int)Side::Black - 1][(int)LateralSide::KingSide] = SQ_H8;
	s_RookCastleSquares[(int)Side::Black - 1][(int)LateralSide::QueenSide] = SQ_A8;

	s_RookCastleDestSquares[(int)Side::White - 1][(int)LateralSide::KingSide] = SQ_F1;
	s_RookCastleDestSquares[(int)Side::White - 1][(int)LateralSide::QueenSide] = SQ_D1;

	s_RookCastleDestSquares[(int)Side::Black - 1][(int)LateralSide::KingSide] = SQ_F8;
	s_RookCastleDestSquares[(int)Side::Black - 1][(int)LateralSide::QueenSide] = SQ_D8;

	// Kings
	s_KingCastleSquares[(int)Side::White - 1][(int)LateralSide::KingSide] = SQ_G1;
	s_KingCastleSquares[(int)Side::White - 1][(int)LateralSide::QueenSide] = SQ_C1;
	s_KingCastleSquares[(int)Side::Black - 1][(int)LateralSide::KingSide] = SQ_G8;
	s_KingCastleSquares[(int)Side::Black - 1][(int)LateralSide::QueenSide] = SQ_C8;

	s_KingDefaultSquares[(int)Side::White - 1] = SQ_E1;
	s_KingDefaultSquares[(int)Side::Black - 1] = SQ_E8;
}

//
//  Chebyshev Distances
//

static int s_ChebyshevDistances[64][64];

static void precomputeChebyshevDistances() {
	for (Square a = 0; a < 64; ++a) {
		for (Square b = 0; b < 64; ++b) {
			int fileA = squares::fileOf(a);
			int fileB = squares::fileOf(b);
			int rankA = squares::rankOf(a);
			int rankB = squares::rankOf(b);

			int rankDist = abs(rankB - rankA);
			int fileDist = abs(fileB - fileA);

			s_ChebyshevDistances[a][b] = std::max(rankDist, fileDist);
		}
	}
}

int getChebyshevDistance(Square a, Square b) {
	return s_ChebyshevDistances[a][b];
}

//
//  Center-Manhattan Distances
//

static const int s_CenterManhattanDistance[64] = {
		6, 5, 4, 3, 3, 4, 5, 6,
		5, 4, 3, 2, 2, 3, 4, 5,
		4, 3, 2, 1, 1, 2, 3, 4,
		3, 2, 1, 0, 0, 1, 2, 3,
		3, 2, 1, 0, 0, 1, 2, 3,
		4, 3, 2, 1, 1, 2, 3, 4,
		5, 4, 3, 2, 2, 3, 4, 5,
		6, 5, 4, 3, 3, 4, 5, 6
};

int getManhattanCenterDistance(Square s) {
	return s_CenterManhattanDistance[s];
}

//
//  Manhattan Distances
//

static int s_ManhattanDistances[64][64];

static void precomputeManhattanDistances() {
	for (Square a = 0; a < 64; ++a) {
		for (Square b = 0; b < 64; ++b) {
			int fileA = squares::fileOf(a);
			int fileB = squares::fileOf(b);
			int rankA = squares::rankOf(a);
			int rankB = squares::rankOf(b);

			int rankDist = abs(rankB - rankA);
			int fileDist = abs(fileB - fileA);

			s_ManhattanDistances[a][b] = rankDist + fileDist;
		}
	}
}

int getManhattanDistance(Square a, Square b) {
	return s_ManhattanDistances[a][b];
}

void initialize() {
	generateCastlingSquares();
	precomputeManhattanDistances();
	precomputeChebyshevDistances();
}

}