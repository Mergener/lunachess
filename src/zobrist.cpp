#include "zobrist.h"

#include <ctime>

namespace lunachess::zobrist {

ui64 g_PieceSquareKeys[PT_COUNT][CL_COUNT][64];
ui64 g_CastlingRightsKey[16];
ui64 g_ColorToMoveKey[2];
ui64 g_EnPassantSquareKey[256];

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
    fillKeysArray(g_PieceSquareKeys, sizeof(g_PieceSquareKeys));
    fillKeysArray(g_CastlingRightsKey, sizeof(g_CastlingRightsKey));
    fillKeysArray(g_ColorToMoveKey, sizeof(g_ColorToMoveKey));
    fillKeysArray(g_EnPassantSquareKey, sizeof(g_EnPassantSquareKey));
}

}