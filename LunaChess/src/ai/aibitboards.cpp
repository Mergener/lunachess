#include "aibitboards.h"

#include <cstring>

namespace lunachess::ai::aibitboards {

static Bitboard s_KnightOPContestantsBB[64][2];

static void precomputeKnightOPContestantsBitboards() {
	// A knight is in an outpost if it cannot be ever contested by an opponent pawn.
	// The bitboards generated here are the squares that an opponent must have pawns in
	// order to contest a knight in a given square sq. Those squares are always squares
	// in files adjacent to the knight's file and in ranks in which an opponent's pawn
	// could still march their way towards the knight's square.
	FOREACH_SIDE(side) {
		int rankDir = side == Side::White ? 1 : -1;

		for (Square sq = 0; sq < 64; ++sq) {
			
			Bitboard bb = 0;

			int file = squares::fileOf(sq);
			for (int f = file - 1; f <= file + 1; f += 2) {
				if (f < 0 || f > 7) {
					continue;
				}

				for (int rank = squares::rankOf(sq) + rankDir; (rank >= 0) && (rank < 8); rank += rankDir) {
					bb.add(rank * 8 + f);
				}
			}

			s_KnightOPContestantsBB[sq][(int)side - 1] = bb;
		}

	}
}

Bitboard getKnightOPContestantsBitboard(Square s, Side side) {
	return s_KnightOPContestantsBB[s][(int)side - 1];
}

static Bitboard s_PawnShieldBitboards[64][2];

static void precomputePawnShields() {
	FOREACH_SIDE(s) {
		int pawnStep = bitboards::getPawnPushStep(s);

		for (int x = 0; x < 8; ++x) {
			for (int y = 0; y < 8; ++y) {
				Square kingSquare = squares::fromPoint(x, y);
				Bitboard bb = 0;

				for (int j = 1; j <= 2; ++j) {
					int py = y + bitboards::getPawnPushDir(s) * j;
					if (py < 0 || py > 7) {
						// Invalid Y
						continue;
					}

					for (int px = x - 1; px <= x + 1; px++) {
						if (px < 0 || px > 7) {
							// Invalid X
							continue;
						}

						bb.add(squares::fromPoint(px, py));
					}
				}

				s_PawnShieldBitboards[kingSquare][(int)s - 1] = bb;
			}			
		}
	}
}

Bitboard getPawnShieldBitboard(Square kingSquare, Side side) {
	return s_PawnShieldBitboards[kingSquare][(int)side - 1];
}

static Bitboard s_PasserContestants[64][2];

static void precomputePasserContestants() {
	FOREACH_SIDE(side) {
		int sideIdx = static_cast<int>(side) - 1;
		for (Square s = 0; s < 64; ++s) {
			Bitboard passerContestants = 0;

			int file = squares::fileOf(s);
			int rank = squares::rankOf(s);
			int yStep = side == Side::White ? 1 : -1;

			for (int j = rank + yStep; j > 0 && j < 7; j += yStep) {
				passerContestants.add(squares::fromPoint(file, j));
			}

			s_PasserContestants[s][sideIdx] = passerContestants;
		}
	}
}

static Bitboard getPasserContestant(Square s, Side side) {
	return s_PawnShieldBitboards[s][(int)side - 1];
}

PawnStructureAnalysis analyzePawnStructure(Bitboard whitePawns, Bitboard blackPawns, Side side) {
	Bitboard ourPawns, theirPawns;
	if (side == Side::White) {
		ourPawns = whitePawns;
		theirPawns = blackPawns;
	}
	else {
		ourPawns = blackPawns;
		theirPawns = whitePawns;
	}

	PawnStructureAnalysis ret;

	// Analyze passed pawns
	for (Square s : ourPawns) {
		auto contestants = getPasserContestant(s, side);
		if ((contestants & theirPawns) == 0) {
			ret.passedPawns.add(s);
		}
	}

	// Analyze connected pawns

}

FileState getFileState(int file, Bitboard whitePawns, Bitboard blackPawns) {
	Bitboard fileBB = bitboards::getFileBitboard(file);

	whitePawns &= fileBB;
	blackPawns &= fileBB;

	if (whitePawns == 0) {
		return blackPawns == 0 ? FileState::Open : FileState::SemiOpen;
	}
	if (blackPawns == 0) {
		return whitePawns == 0 ? FileState::Open : FileState::SemiOpen;
	}
	return FileState::Closed;
}

void initialize() {
	precomputeKnightOPContestantsBitboards();
	precomputePawnShields();
	precomputePasserContestants();
}

}