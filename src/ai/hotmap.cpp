#include "hotmap.h"

#include <cstring>

#include "../serial.h"

namespace lunachess::ai {

KingSquareHotmapGroup Hotmap::defaultMiddlegameMaps[PT_COUNT - 1];
KingSquareHotmapGroup Hotmap::defaultEndgameMaps[PT_COUNT - 1];
Hotmap Hotmap::defaultKingEgHotmap;
Hotmap Hotmap::defaultKingMgHotmap;

void Hotmap::initializeHotmaps() {
    // Initialize everything to zero by default
    std::memset(defaultMiddlegameMaps, 0, sizeof(defaultMiddlegameMaps));
    std::memset(defaultEndgameMaps, 0, sizeof(defaultEndgameMaps));

    //
    // Pawns
    //
    defaultMiddlegameMaps[PT_PAWN] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        300, 300, 300, 300, 300, 300, 300, 300,
        142, 142, 163, 205, 205, 172, 160, 142,
        76, 80, 140, 191, 191, 72, 68, 90,
        32, 42, 98, 162, 162, 43, 24, 32,
        32, 19, 58, 70, 70, -36, 0, 24,
        0, 0, -42, -60, -50, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,

    };

    defaultEndgameMaps[PT_PAWN] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        550, 500, 500, 500, 500, 500, 500, 550,
        350, 300, 300, 300, 300, 300, 300, 350,
        250, 200, 200, 200, 200, 200, 200, 250,
        150, 100, 100, 100, 100, 100, 100, 150,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    };

    //
    // Knights
    //
    defaultMiddlegameMaps[PT_KNIGHT] = {
        70, 90, 110, 110, 110, 110, 90, 70,
        70, 90, 110, 110, 110, 110, 90, 70,
        70, 90, 110, 110, 110, 110, 90, 70,
        70, 90, 110, 110, 110, 110, 90, 70,
        50, 70, 90, 90, 90, 90, 70, 50,
        40, 70, 70, 70, 70, 105, 70, 70,
        30, 60, 60, 60, 60, 60, 60, 30,
        0, -6, 0, 0, 0, 0, -12, 0
    };

    defaultEndgameMaps[PT_KNIGHT] = {
        10, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 10, 10, 10, 10, 10, 10,
        -90, 0, 0, 0, 0, 0, 0, -90,
        -90, 0, 0, 0, 0, 0, 0, -90,
        -90, 0, 0, 0, 0, 0, 0, -90,
        -90, -90, -90, -90, -90, -90, -90, -90,
    };

    //
    // Bishops
    //
    defaultMiddlegameMaps[PT_BISHOP] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    defaultEndgameMaps[PT_BISHOP] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    //
    // Rooks
    //
    defaultMiddlegameMaps[PT_ROOK] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        50, 50, 50, 50, 50, 50, 50, 50,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    defaultEndgameMaps[PT_ROOK] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        50, 50, 50, 50, 50, 50, 50, 50,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    //
    // Queens
    //
    defaultMiddlegameMaps[PT_QUEEN] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        -50, -50, -50, -70, -70, -50, -50, -50,
        -50, -50, -50, -70, -70, -50, -50, -50,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        10, 10, 10, 10, 10, 10, 10, 10
    };

    defaultEndgameMaps[PT_QUEEN] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    //
    // Kings
    //
    defaultKingMgHotmap = {
        -300, -300, -300, -300, -300, -300, -300, -300,
        -300, -300, -300, -300, -300, -300, -300, -300,
        -300, -300, -300, -300, -300, -300, -300, -300,
        -300, -300, -300, -300, -300, -300, -300, -300,
        -300, -300, -300, -300, -300, -300, -300, -300,
        -300, -300, -300, -300, -300, -300, -300, -300,
        0, 0, -60, -120, -120, -60, 0, 0,
        60, 60, 0, -60, -60, 0, 60, 60
    };

    defaultKingEgHotmap = {
        -200, -200, -200, -200, -200, -200, -200, -200,
        -200, -100, -100, -100, -100, -100, -100, -200,
        -200, -100, 0, 0, 0, 0, -100, -200,
        -200, -100, 0, 100, 100, 0, -100, -200,
        -200, -100, 0, 100, 100, 0, -100, -200,
        -200, -100, 0, 0, 0, 0, -100, -200,
        -200, -100, -100, -100, -100, -100, -100, -200,
        -200, -200, -200, -200, -200, -200, -200, -200,
    };
}
std::istream& operator>>(std::istream& stream, Hotmap& hotmap) {
    SerialValue val;
    stream >> val;

    auto list = val.get<SerialList>();
    for (int i = 0; i < 64; ++i) {
        if (list.size() >= i) {
            break;
        }
        hotmap.m_Values[i] = list[i].get<i32>();
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const Hotmap& hotmap) {
    SerialValue val(hotmap.m_Values.begin(), hotmap.m_Values.end());
    stream << val;
    return stream;
}

}