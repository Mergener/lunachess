#include "bitboard.h"

#include <iostream>

#include "staticlist.h"
#include "bits.h"
#include "defs.h"

namespace lunachess {

std::ostream& operator<<(std::ostream& stream, Bitboard b) {
	for (i64 j = 7; j >= 0; --j) {
		stream << "[ ";
		
		for (i64 i = 0; i < 8; ++i) {
			ui64 bit = j * 8 + i;

			if ((b & (C64(1) << bit))) {
				stream << "1";
			} else {
				stream << ".";
			}
			stream << " ";
			bit--;
		}

		stream << "]\n";
	}
	return stream;
}

static int s_BitboardsBitCount[256];

int Bitboard::count() const {
	return (s_BitboardsBitCount[m_BB & 0xff]
		+ s_BitboardsBitCount[(m_BB >> 8) & 0xff]
		+ s_BitboardsBitCount[(m_BB >> 16) & 0xff]
		+ s_BitboardsBitCount[(m_BB >> 24) & 0xff]
		+ s_BitboardsBitCount[(m_BB >> 32) & 0xff]
		+ s_BitboardsBitCount[(m_BB >> 40) & 0xff]
		+ s_BitboardsBitCount[(m_BB >> 48) & 0xff]
		+ s_BitboardsBitCount[(m_BB >> 56) & 0xff]);
}

Bitboard::Iterator::Iterator(const Bitboard& board, i8 bit) {
	m_Bit = bit;
	m_BB = board.m_BB;
	if (m_Bit != 64 && !bits::bitScanF(m_BB >> m_Bit, m_Bit)) {
		m_Bit = 64;
	}
}

Bitboard::Iterator::Iterator(const Iterator& it) noexcept {
	m_BB = it.m_BB;
	m_Bit = it.m_Bit;
}

Bitboard::Iterator::Iterator(Bitboard::Iterator&& it) noexcept {
	m_BB = it.m_BB;
	m_Bit = it.m_Bit;
}

Bitboard::Iterator& Bitboard::Iterator::operator++() {
	m_Bit++;
	i8 delta;

	if (m_Bit != 64) {
		if (!bits::bitScanF(m_BB >> m_Bit, delta)) {
			m_Bit = 64;
		}
		else {
			m_Bit += delta;
		}
	}

	return *this;
}

bool Bitboard::Iterator::operator==(const Iterator& it) {
	return m_Bit == it.m_Bit;
}

bool Bitboard::Iterator::operator!=(const Iterator& it) {
	return m_Bit != it.m_Bit;
}
Square Bitboard::Iterator::operator*() {
	return static_cast<Square>(m_Bit);
}

namespace bitboards {

BITWISE_ENUM_CLASS(RayDirection, ui8,
	North,
	NorthEast,
	East,
	SouthEast,
	South,
	SouthWest,
	West,
	NorthWest
);

static Bitboard s_RayAttacks[8][64];
static Bitboard s_KnightAttacks[64];
static Bitboard s_KingAttacks[64];
static Bitboard s_PawnPushes[2][64];
static Bitboard s_PawnCaptures[2][64];
static bool s_Initialized = false;

static inline Square step(Square sqr, RayDirection dir, int nSteps = 1) {
	static Square dirDeltas[] = { 8, 9, 1, -7, -8, -9, -1, 7 };

	return sqr + dirDeltas[(int)dir] * nSteps;
}

static const char* getDirName(RayDirection dir) {
	static const char* names[] = {
		"North", "Northeast", "East",
		"Southeast", "South", "Southwest",
		"West", "Northwest"
	};

	return names[(int)dir];
}

static void precomputeRayAttacks() {
	for (Square s = 0; s < 64; ++s) {
		for (int d = 0; d < 8; ++d) {
			RayDirection dir = static_cast<RayDirection>(d);

			Bitboard attacks = 0; // Initialize with zero

			int bit = step(s, dir);
			int x = bit % 8;
			int y = bit / 8;
			int initialX = s % 8;
			int initialY = s / 8;

			while (true) {
				bool exitLoop = false;

				switch (dir) {
				case RayDirection::North:
				case RayDirection::South:
					if (x != initialX) {
						exitLoop = true;
					}
					break;

				case RayDirection::East:
				case RayDirection::West:
					if (y != initialY) {
						exitLoop = true;
					}
					break;

				case RayDirection::SouthEast:
					if (y >= initialY || x <= initialX) {
						exitLoop = true;
					}
					break;

				case RayDirection::SouthWest:
					if (y >= initialY || x >= initialX) {
						exitLoop = true;
					}
					break;

				case RayDirection::NorthEast:
					if (y <= initialY || x <= initialX) {
						exitLoop = true;
					}
					break;

				case RayDirection::NorthWest:
					if (y <= initialY || x >= initialX) {
						exitLoop = true;
					}
					break;
				}

				if (exitLoop || (bit < 0) || (bit >= 64)) {
					break;
				}

				attacks |= (C64(1) << bit);
				bit = step(bit, dir);
				x = bit % 8;
				y = bit / 8;
			}

			s_RayAttacks[(int)dir][s] = attacks;
		}
	}
}

static void precomputeKnightAttacks() {
	for (Square s = 0; s < 64; ++s) {
		Bitboard b = 0;

		int x = s % 8;
		int y = s / 8;

		if (x > 0 && y < 6) {
			b |= C64(1) << step(step(s, RayDirection::North), RayDirection::NorthWest);
		}

		if (x < 7 && y < 6) {
			b |= C64(1) << step(step(s, RayDirection::North), RayDirection::NorthEast);
		}

		if (x < 6 && y < 7) {
			b |= C64(1) << step(step(s, RayDirection::NorthEast), RayDirection::East);
		}

		if (x < 6 && y > 0) {
			b |= C64(1) << step(step(s, RayDirection::SouthEast), RayDirection::East);
		}

		if (x < 7 && y > 1) {
			b |= C64(1) << step(step(s, RayDirection::South), RayDirection::SouthEast);
		}

		if (x > 0 && y > 1) {
			b |= C64(1) << step(step(s, RayDirection::South), RayDirection::SouthWest);
		}

		if (x > 1 && y > 0) {
			b |= C64(1) << step(step(s, RayDirection::SouthWest), RayDirection::West);
		}

		if (x > 1 && y < 7) {
			b |= C64(1) << step(step(s, RayDirection::NorthWest), RayDirection::West);
		}

		s_KnightAttacks[s] = b;
	}
}

static void precomputePawnAttacks() {
	std::memset(s_PawnCaptures, 0, sizeof(s_PawnCaptures));
	std::memset(s_PawnPushes, 0, sizeof(s_PawnPushes));

	FOREACH_SIDE(side) {
		int yStep, initialRank;
		
		if (side == Side::White) {
			yStep = 1;
			initialRank = 1;
		}
		else {
			yStep = -1;
			initialRank = 6;
		}

		for (int x = 0; x < 8; ++x) {
			for (int y = 1; y < 7; ++y) {
				int sideIdx = static_cast<int>(side) - 1;
				Square sq = squares::fromPoint(x, y);

				// Do pushes:
				Bitboard pushes = 0;
				pushes.add(squares::fromPoint(x, y + yStep));
				if (y == initialRank) {
					// Add double pawn push
					pushes.add(squares::fromPoint(x, y + yStep * 2));
				}

				// Do captures
				Bitboard captures = 0;
				if (x < 7) {
					captures.add(squares::fromPoint(x + 1, y + yStep));
				}
				if (x > 0) {
					captures.add(squares::fromPoint(x - 1, y + yStep));
				}

				s_PawnPushes[sideIdx][sq] = pushes;
				s_PawnCaptures[sideIdx][sq] = captures;
			}
		}
	}
}

static void precomputeKingAttacks() {
	for (Square s = 0; s < 64; ++s) {
		s_KingAttacks[s] = getPieceAttacks(PieceType::Queen, UINT64_MAX, s, Side::White);
	}
}

static Bitboard s_KingCastlingPaths[2][2];
static Bitboard s_RookCastlingPaths[2][2];

static void generateCastlingPaths() {
	// White king-side
	Bitboard wk = 0;
	wk.add(SQ_E1);
	wk.add(SQ_F1);
	wk.add(SQ_G1);
	s_KingCastlingPaths[(int)Side::White - 1][(int)LateralSide::KingSide] = wk;

	Bitboard wkr = 0;
	wkr.add(SQ_H1);
	wkr.add(SQ_G1);
	wkr.add(SQ_F1);
	s_RookCastlingPaths[(int)Side::White - 1][(int)LateralSide::KingSide] = wkr;

	// White queen-side
	Bitboard wq = 0;
	wq.add(SQ_E1);
	wq.add(SQ_D1);
	wq.add(SQ_C1);
	s_KingCastlingPaths[(int)Side::White - 1][(int)LateralSide::QueenSide] = wq;

	Bitboard wqr = 0;
	wqr.add(SQ_A1);
	wqr.add(SQ_B1);
	wqr.add(SQ_C1);
	wqr.add(SQ_D1);
	s_RookCastlingPaths[(int)Side::White - 1][(int)LateralSide::QueenSide] = wqr;

	// Black king-side
	Bitboard bk = 0;
	bk.add(SQ_E8);
	bk.add(SQ_F8);
	bk.add(SQ_G8);
	s_KingCastlingPaths[(int)Side::Black - 1][(int)LateralSide::KingSide] = bk;

	Bitboard bkr = 0;
	bkr.add(SQ_H8);
	bkr.add(SQ_G8);
	bkr.add(SQ_F8);
	s_RookCastlingPaths[(int)Side::Black - 1][(int)LateralSide::QueenSide] = bkr;

	// Black queen-side
	Bitboard bq = 0;
	bq.add(SQ_E8);
	bq.add(SQ_D8);
	bq.add(SQ_C8);
	s_KingCastlingPaths[(int)Side::Black - 1][(int)LateralSide::QueenSide] = bq;

	Bitboard bqr = 0;
	bqr.add(SQ_A8);
	bqr.add(SQ_B8);
	bqr.add(SQ_C8);
	bqr.add(SQ_D8);
	s_RookCastlingPaths[(int)Side::Black - 1][(int)LateralSide::QueenSide] = bqr;
}

Bitboard getCastlingKingPath(Side side, LateralSide lSide) {
	return s_KingCastlingPaths[(int)side - 1][(int)lSide];
}


Bitboard getCastlingRookPath(Side side, LateralSide lSide) {
	return s_RookCastlingPaths[(int)side - 1][(int)lSide];
}

static Bitboard getPositiveRayAttacks(Bitboard occupancy, RayDirection dir, Square srcSqr) {
	Bitboard attacks = s_RayAttacks[(int)dir][srcSqr];
	Bitboard blocker = attacks & occupancy;
	bits::bitScanF(blocker | C64(0x8000000000000000), srcSqr);
	return attacks ^ s_RayAttacks[(int)dir][srcSqr];
}

static Bitboard getNegativeRayAttacks(Bitboard occupancy, RayDirection dir, Square srcSqr) {
	Bitboard attacks = s_RayAttacks[(int)dir][srcSqr];
	Bitboard blocker = attacks & occupancy;
	bits::bitScanR(blocker | 1, srcSqr);
	return attacks ^ s_RayAttacks[(int)dir][srcSqr];
}

Bitboard getDiagonalAttacks(Bitboard occupancy, Square sqr) {
	return getPositiveRayAttacks(occupancy, RayDirection::NorthEast, sqr)
		| getNegativeRayAttacks(occupancy, RayDirection::SouthWest, sqr);
}

Bitboard getAntiDiagonalAttacks(Bitboard occupancy, Square sqr) {
	return getPositiveRayAttacks(occupancy, RayDirection::NorthWest, sqr)
		| getNegativeRayAttacks(occupancy, RayDirection::SouthEast, sqr);
}

Bitboard getFileAttacks(Bitboard occupancy, Square sqr) {
	return getPositiveRayAttacks(occupancy, RayDirection::North, sqr)
		| getNegativeRayAttacks(occupancy, RayDirection::South, sqr);
}

Bitboard getRankAttacks(Bitboard occupancy, Square sqr) {
	return getPositiveRayAttacks(occupancy, RayDirection::East, sqr)
		| getNegativeRayAttacks(occupancy, RayDirection::West, sqr);
}

Bitboard getRookAttacks(Bitboard occupancy, Square sqr, Side side) {
	auto ret = getFileAttacks(occupancy, sqr) | getRankAttacks(occupancy, sqr);
	return ret;
}

Bitboard getBishopAttacks(Bitboard occupancy, Square sqr, Side side) {
	return getDiagonalAttacks(occupancy, sqr) | getAntiDiagonalAttacks(occupancy, sqr);
}

Bitboard getQueenAttacks(Bitboard occupancy, Square sqr, Side side) {
	return getRookAttacks(occupancy, sqr, side) | getBishopAttacks(occupancy, sqr, side);
}

Bitboard getKnightAttacks(Square sqr) {
	return s_KnightAttacks[sqr];
}

Bitboard getKnightAttacks(Bitboard occupancy, Square sqr, Side side) {
	return getKnightAttacks(sqr);
}

Bitboard getPawnAttacks(Bitboard occupancy, Square sqr, Side side) {
	// For double pushes, the occupancy of the square right in front of the pawn
	// must also be considered occupancy of the square of distance 2 ahead of the pawn.
	i8 pushStep = getPawnPushStep(side);

	// One square towards the pawn's path
	i8 singlePushSq = sqr + pushStep;

	// Occupancy at the square right in front of the pawn
	ui64 singlePushOcc = occupancy & (C64(1) << singlePushSq);

	// Two square towards the pawn's path
	i8 doublePushSq = sqr + 2 * pushStep;

	occupancy |= (singlePushOcc >> singlePushSq) << doublePushSq;

	// For pushes, exclude occupied squares
	Bitboard pushes = s_PawnPushes[(int)side - 1][sqr] & (~occupancy);

	// For captures, only include occupied squares
	Bitboard captures = s_PawnCaptures[(int)side - 1][sqr];
	pushes |= (captures & occupancy);

	return pushes;
}

Bitboard getKingAttacks(Bitboard occupancy, Square sqr, Side side) {
	return s_KingAttacks[sqr];
}

using PieceAttacksFunc = Bitboard(*)(Bitboard, Square, Side);

static PieceAttacksFunc s_AttacksFuncs[static_cast<int>(PieceType::_Count)];

static Bitboard getEmptyAttacks(Bitboard occupancy, Square sqr, Side side) {
	return 0;
}

static void generateAttackFuncsArray() {
	std::fill(s_AttacksFuncs, s_AttacksFuncs + sizeof(s_AttacksFuncs) / sizeof(PieceAttacksFunc), getEmptyAttacks);

	s_AttacksFuncs[(int)PieceType::Bishop] = getBishopAttacks;
	s_AttacksFuncs[(int)PieceType::Rook] = getRookAttacks;
	s_AttacksFuncs[(int)PieceType::Queen] = getQueenAttacks;
	s_AttacksFuncs[(int)PieceType::Knight] = getKnightAttacks;
	s_AttacksFuncs[(int)PieceType::Pawn] = getPawnAttacks;
	s_AttacksFuncs[(int)PieceType::King] = getKingAttacks;
}

Bitboard getPieceAttacks(PieceType pieceType, Bitboard occupancy, Square sqr, Side side) {
	ui64 val = (s_AttacksFuncs[(int)pieceType](occupancy, sqr, side));
	return val;
}

static Bitboard s_FileBitboards[8];
static Bitboard s_RankBitboards[8];

static void precomputeRanksAndFiles() {
	for (int i = 0; i < 8; ++i) {
		Bitboard fileBB;
		Bitboard rankBB;

		for (int j = 0; j < 8; ++j) {
			fileBB.add(j * 8 + i);
			rankBB.add(i * 8 + j);
		}

		s_FileBitboards[i] = fileBB;
		s_RankBitboards[i] = rankBB;
	}
}

Bitboard getFileBitboard(int file) {
	return s_FileBitboards[file];
}

Bitboard getRankBitboard(int rank) {
	return s_RankBitboards[rank];
}

static Bitboard s_LightSquares = 0;
static Bitboard s_DarkSquares = 0;

static void precomputeSquareColors() {
	bool lightSquare = false;
	for (int j = 0; j < 8; ++j) {
		for (int i = 0; i < 8; ++i) {
			if (lightSquare) {
				s_LightSquares.add(i * 8 + j);
			}
			lightSquare = !lightSquare;
		}
		lightSquare = !lightSquare;
	}
	s_DarkSquares = ~s_LightSquares;
}

Bitboard getLightSquares() {
	return s_LightSquares;
}

Bitboard getDarkSquares() {
	return s_DarkSquares;
}

void initialize() {
	if (s_Initialized) {
		return;
	}
	s_Initialized = true;

	s_BitboardsBitCount[0] = 0;
	for (int i = 0; i < 256; i++)
	{
		s_BitboardsBitCount[i] = (i & 1) + s_BitboardsBitCount[i / 2];
	}

	generateAttackFuncsArray();
	generateCastlingPaths();

	precomputeRayAttacks();
	precomputeKnightAttacks();
	precomputePawnAttacks();
	precomputeKingAttacks();
	precomputeRanksAndFiles();
	precomputeSquareColors();
}

} // namespace bitboards

} // namespace lunachess