#include "../tests.h"

#include <functional>
#include "core/bitboard.h"

namespace lunachess::tests {

static struct {

	std::function<Bitboard()> call;
	Bitboard expected;

} s_Tests[] = {

	// Get a visualization for bitboards at: 
	//	https://tearth.dev/bitboard-viewer/ (lunachess uses layout 1)

	//
	// Pawns:
	//
	{ []() { return bitboards::getPieceAttacks(WHITE_PAWN, 0x0, SQ_A2); }, 0x1010000 },
	{ []() { return bitboards::getPieceAttacks(BLACK_PAWN, 0x0, SQ_A2); }, 0x1 },
	{ []() { return bitboards::getPieceAttacks(WHITE_PAWN, 0x1, SQ_A2); }, 0x1010000 },
	{ []() { return bitboards::getPieceAttacks(BLACK_PAWN, 0x1, SQ_A2); }, 0x0 },
	// The following cases test if a pawn on the rightmost and leftmost ranks is able to capture
	// a piece on the eigth/first rank on the other side of the board (they shouldn't)
	{ []() { return bitboards::getPieceAttacks(WHITE_PAWN, 0x4100000000000000, SQ_H7); }, 0xc000000000000000 },
	{ []() { return bitboards::getPieceAttacks(WHITE_PAWN, 0x8200000000000000, SQ_A7); }, 0x300000000000000 },
	{ []() { return bitboards::getPieceAttacks(BLACK_PAWN, 0x41, SQ_H2); }, 0xc0 },
	{ []() { return bitboards::getPieceAttacks(BLACK_PAWN, 0x82, SQ_A2); }, 0x3 },

	//
	// Rooks:
	//
	{ []() { return bitboards::getPieceAttacks(WHITE_ROOK, 0x0, SQ_E4); }, 0x10101010ef101010 },
	{ []() { return bitboards::getPieceAttacks(WHITE_ROOK, 0x30120008044010, SQ_E4); }, 0x1010e8101010 },

	//
	// Knights:
	//
	{ []() { return bitboards::getPieceAttacks(WHITE_KNIGHT, 0x0, SQ_B2); }, 0x5080008 },
	{ []() { return bitboards::getPieceAttacks(WHITE_KNIGHT, 0x33, SQ_B2); }, 0x5080008 },
	{ []() { return bitboards::getPieceAttacks(WHITE_KNIGHT, UINT64_MAX, SQ_B2); }, 0x5080008 },

	//
	// Kings:
	//
	{ []() { return bitboards::getPieceAttacks(WHITE_KING, 0x4000, SQ_H1); }, 0xc040 }

};

void testBitboards() {
	constexpr int COUNT = sizeof(s_Tests) / sizeof(*s_Tests);

	for (int i = 0; i < COUNT; ++i) {
		const auto& test = s_Tests[i];

		Bitboard got = test.call();

		LUNA_ASSERT(got == test.expected, "Wrong bitboard result (index " << i << "). Expected:\n\n" << test.expected <<
			"\nGot:\n\n" << got);
	}
}

}