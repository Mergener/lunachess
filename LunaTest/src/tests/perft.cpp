#include "../tests.h"

#include "core/position.h"

namespace lunachess::tests {

static struct {
	const char* fen;
	int depth;
	ui64 expectedLeaves = UINT64_MAX;
	ui64 expectedCaptures = UINT64_MAX;
	ui64 expectedEps = UINT64_MAX;
	ui64 expectedCastles = UINT64_MAX;

} s_PerftPositions[] = {

	// Data taken from https://www.chessprogramming.org/Perft_Results

	// Initial position PERFT:
	{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ", 1, 20, 0, 0, 0 },
	{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ", 2, 400, 0, 0, 0 },
	{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ", 3, 8902, 34, 0, 0 },
	{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ", 4, 197281, 1576, 0, 0 },

	// Pos #2 (Kiwipete)
	{ "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ", 1, 48, 8, 0, 2 },
	{ "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ", 2, 2039, 351, 1, 91 },
	{ "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ", 3, 97862, 17102, 45, 3162 },

	// Pos #3
	{ "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ", 1, 14, 1, 0, 0 },
	{ "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ", 2, 191, 14, 0, 0 },
	{ "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ", 3, 2812, 209, 2, 0 },
	{ "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ", 4, 43238, 3348, 123, 0 },
};

void testPerft() {
	constexpr int COUNT = sizeof(s_PerftPositions) / sizeof(*s_PerftPositions);

	for (int i = 0; i < COUNT; ++i) {
		auto perftPos = s_PerftPositions[i];
		Position pos;
		PerftStats stats;

		pos = Position::fromFEN(s_PerftPositions[i].fen).value();

		pos.richPerft(perftPos.depth, stats);

		LUNA_ASSERT(stats.leafNodes == perftPos.expectedLeaves,
			"Wrong perft result. Expected " << perftPos.expectedLeaves << ", got " << stats.leafNodes << "." <<
			" (perft test index " << i << ")");

		LUNA_ASSERT(stats.captures == perftPos.expectedCaptures,
			"Wrong perft result for captures. Expected " << perftPos.expectedCaptures << ", got " << stats.captures << "." <<
			" (perft test index " << i << ")");

		LUNA_ASSERT(stats.castles == perftPos.expectedCastles,
			"Wrong perft result for castles. Expected " << perftPos.expectedCastles << ", got " << stats.castles << "." <<
			" (perft test index " << i << ")");

		LUNA_ASSERT(stats.enPassants == perftPos.expectedEps,
			"Wrong perft result for en-passants. Expected " << perftPos.expectedEps << ", got " << stats.enPassants << "." <<
			" (perft test index " << i << ")");
	}
}

}