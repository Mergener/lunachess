#include "hotmap.h"

namespace lunachess::ai {

KingSquareHotmapGroup Hotmap::defaultMiddlegameMaps[PT_COUNT - 1];
KingSquareHotmapGroup Hotmap::defaultEndgameMaps[PT_COUNT - 1];
Hotmap Hotmap::defaultKingEgHotmap;
Hotmap Hotmap::defaultKingMgHotmap;

Hotmap Hotmap::defaultMgPasserHotmap;
Hotmap Hotmap::defaultMgChainedPawnHotmap;
Hotmap Hotmap::defaultMgConnectedPasserHotmap;
Hotmap Hotmap::defaultEgPasserHotmap;
Hotmap Hotmap::defaultEgChainedPawnHotmap;
Hotmap Hotmap::defaultEgConnectedPasserHotmap;

void Hotmap::initializeHotmaps() {
    //
    // Pawns
    //
    defaultMiddlegameMaps[PT_PAWN] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        500, 450, 450, 450, 450, 450, 450, 500,
        250, 250, 250, 250, 250, 250, 250, 300,
        0, 0, 60, 200, 200, 0, 0, 0,
        0, 0, 60, 190, 190, 0, 0, 0,
        0, 0, 40, 100, 100, 0, 0, 0,
        0, 0, -30, -100, -100, 0, 0, 0,
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
        10, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 10, 10, 10, 10, 10, 10,
        -20, 0, 0, 0, 0, 0, 0, -20,
        -90, 0, 0, 0, 0, 0, 0, -90,
        -90, 0, 0, 0, 0, 0, 0, -90,
        -90, -90, -90, -90, -90, -90, -90, -90,
    };

    defaultEndgameMaps[PT_KNIGHT] = {
        10, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 10, 10, 10, 10, 10, 10,
        -20, 0, 0, 0, 0, 0, 0, -20,
        -90, 0, 0, 0, 0, 0, 0, -90,
        -90, 0, 0, 0, 0, 0, 0, -90,
        -90, -90, -90, -90, -90, -90, -90, -90,
    };

    //
    // Bishops
    //
    defaultMiddlegameMaps[PT_BISHOP] = {
        72, 0, 0, 0, 0, 0, 0, 72,
        0, 72, 0, 0, 0, 0, 72, 0,
        0, 0, 72, 0, 0, 72, 0, 0,
        0, 0, 0, 72, 72, 0, 0, 0,
        0, 0, 0, 72, 72, 0, 0, 0,
        0, 0, 72, 0, 0, 72, 0, 0,
        0, 72, 0, 0, 0, 0, 72, 0,
        -33,  -3, -14, -21, -13, -12, -39, -21,
    };

    defaultEndgameMaps[PT_BISHOP] = {
        72, 0, 0, 0, 0, 0, 0, 72,
        0, 72, 0, 0, 0, 0, 72, 0,
        0, 0, 72, 0, 0, 72, 0, 0,
        0, 0, 0, 72, 72, 0, 0, 0,
        0, 0, 0, 72, 72, 0, 0, 0,
        0, 0, 72, 0, 0, 72, 0, 0,
        0, 72, 0, 0, 0, 0, 72, 0,
        72, 0, 0, 0, 0, 0, 0, 72
    };

    //
    // Rooks
    //
    defaultMiddlegameMaps[PT_ROOK] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        200, 200, 200, 200, 200, 200, 200, 200,
        0, 0, 50, 75, 75, 0, 0, 0,
        0, 0, 50, 75, 75, 0, 0, 0,
        0, 0, 50, 75, 75, 0, 0, 0,
        0, 0, 50, 75, 75, 0, 0, 0,
        0, 0, 50, 75, 75, 0, 0, 0,
        0, 0, 50, 75, 75, 0, 0, 0,
    };

    defaultEndgameMaps[PT_ROOK] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        200, 200, 200, 200, 200, 200, 200, 200,
        0, 0, 50, 75, 75, 0, 0, 0,
        0, 0, 50, 75, 75, 0, 0, 0,
        0, 0, 50, 75, 75, 0, 0, 0,
        0, 0, 50, 75, 75, 0, 0, 0,
        0, 0, 50, 75, 75, 0, 0, 0,
        0, 0, 50, 75, 75, 0, 0, 0,
    };

    //
    // Queens
    //
    defaultMiddlegameMaps[PT_QUEEN] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        -50, -50, -50, -70, -70, -50, -50, -50,
        -50, -50, -50, -80, -80, -50, -50, -50,
        0, -10, -10, -26, -26, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        10, 10, 10, 15, 15, 10, 10, 10
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
        0, 0, -60, -190, -190, -60, 0, 0,
        100, 100, 30, -120, -120, -50, 100, 100
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

    //
    // Passed pawns
    //

    defaultMgPasserHotmap = {
        0, 0, 0, 0, 0, 0, 0, 0,
        110,110,110,110,110,110,110,110,
        70,70,70,70,70,70,70,70,
        50,50,50,50,50,50,50,50,
        40, 40,40,40,40,40,40,40,
        35, 35, 35, 35, 35, 35, 35, 35,
        35, 35, 35, 35, 35, 35, 35, 35,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    defaultEgPasserHotmap = {
        0, 0, 0, 0, 0, 0, 0, 0,
        230,230,230,230,230,230,230,230,
        170,170,170,170,170,170,170,170,
        120,120,120,120,120,120,120,120,
        80,80,80,80,80,80,80,80,
        60,60,60,60,60,60,60,60,
        60,60,60,60,60,60,60,60,
        0, 0, 0, 0, 0, 0, 0, 0
    };
}

}