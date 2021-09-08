#include "tests.h"

namespace lunachess::tests {

// Test function headers:
void testBitboards();
void testPositions();
void testPerft();
void testStaticList();
void testPieces();

// Test array:
const Test g_Tests[] = {
	Test("Bitboard Tests", testBitboards),
	Test("Position Tests", testPositions),
	Test("Piece Tests", testPieces),
	Test("Perft Tests", testPerft),
	Test("Static List Tests", testStaticList)
};

const std::size_t g_TestCount = sizeof(g_Tests) / sizeof(*g_Tests);

}