#include "zobrist.h"

#include <cstdlib>
#include <ctime>

#include "bits.h"

namespace lunachess::zobrist {

static ui64 s_PieceSquareKeys[(int)PieceType::_Count][3][64];
static ui64 s_CastlingRightsKey[16];
static ui64 s_SideToMoveKey[2];
static ui64 s_EnPassantSquareKey[64];

ui64 getPieceSquareKey(Piece piece, Square sqr) {
	return s_PieceSquareKeys[(int)piece.getType()][(int)piece.getSide()][sqr];
}

ui64 getCastlingRightsKey(CastlingRightsMask crm) {
	return s_CastlingRightsKey[(int)crm];
}

ui64 getSideToMoveKey(Side side) {
	return s_SideToMoveKey[(int)side - 1];
}

ui64 getEnPassantSquareKey(Square sqr) {
	return s_EnPassantSquareKey[sqr];
}

static struct {

	ui8 a = 166;
	ui8 b = 124;
	ui8 c = 13;
	ui8 d = 249;

} s_RandomContext;

static ui8 randomUI8() {
	ui8 e = s_RandomContext.a - bits::rotateLeft(s_RandomContext.b, 7);
	s_RandomContext.a = s_RandomContext.b ^ bits::rotateLeft(s_RandomContext.c, 13);
	s_RandomContext.b = s_RandomContext.c + bits::rotateLeft(s_RandomContext.d, 37);
	s_RandomContext.c = s_RandomContext.d + e;
	s_RandomContext.d = e + s_RandomContext.a;
	return s_RandomContext.d;
}

static ui64 randomUI64() {
	return (static_cast<ui64>(randomUI8()) << 56) |
		   (static_cast<ui64>(randomUI8()) << 48) |
		   (static_cast<ui64>(randomUI8()) << 40) |
		   (static_cast<ui64>(randomUI8()) << 32) |
		   (static_cast<ui64>(randomUI8()) << 24) |
		   (static_cast<ui64>(randomUI8()) << 16) |
		   (static_cast<ui64>(randomUI8()) << 8) |
		   (static_cast<ui64>(randomUI8()));
}

static void fillKeysArray(void* arr, size_t size) {
	ui64* it = reinterpret_cast<ui64*>(arr);
	ui64* last = it + size / sizeof(ui64);

	while (it != last) {
		*it = randomUI64();

		it++;
	}
}

void initialize() {
	// Initialize piece square keys
	fillKeysArray(s_PieceSquareKeys, sizeof(s_PieceSquareKeys));
	fillKeysArray(s_CastlingRightsKey, sizeof(s_CastlingRightsKey));
	fillKeysArray(s_SideToMoveKey, sizeof(s_SideToMoveKey));
	fillKeysArray(s_EnPassantSquareKey, sizeof(s_EnPassantSquareKey));
}
}