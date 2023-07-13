#include "hceweights.h"

namespace lunachess::ai {

PieceSquareTable g_DEFAULT_PAWN_PST_MG_KK = {
        0, 0, 0, 0, 0, 0, 0, 0,
        300, 300, 300, 500, 500, 300, 300, 300,
        250, 250, 300, 500, 500, 300, 300, 300,
        100, 80, 0, 410, 410, 0, 0, 150,
        50, 50, 100, 375, 375, -80, -160, -20,
        25, 25, 80, 250, 250, -100, 80, 90,
        0, 0, -50, -100, -100, 125, 160, 60,
        0, 0, 0, 0, 0, 0, 0, 0,
};

PieceSquareTable g_DEFAULT_PAWN_PST_MG_KQ = {
        0, 0, 0, 0, 0, 0, 0, 0,
        350, 350, 300, 500, 500, 300, 300, 300,
        0, 0, 300, 500, 500, 300, 300, 300,
        -100, -100, 200, 410, 410, 0, 0, 0,
        -100, -100, 0, 375, 375, -80, 100, 100,
        -150, -150, 80, 250, 250, 50, 150, 100,
        -200, -200, -150, -100, -100, 150, 260, 250,
        0, 0, 0, 0, 0, 0, 0, 0,
};

PieceSquareTable g_DEFAULT_PAWN_PST_MG_QQ = {
        0, 0, 0, 0, 0, 0, 0, 0,
        300, 300, 300, 500, 500, 300, 300, 300,
        300, 300, 300, 500, 500, 300, 250, 250,
        150, 0, 0, 410, 410, 0, 80, 100,
        -20, -160, -80, 375, 375, 100, 50, 50,
        90, 80, -100, 250, 250, 80, 25, 25,
        60, 160, 125, -100, -100, -50, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
};

PieceSquareTable g_DEFAULT_PAWN_PST_MG_QK = {
        0, 0, 0, 0, 0, 0, 0, 0,
        300, 300, 300, 500, 500, 300, 350, 350,
        300, 300, 300, 500, 500, 300, 0, 0,
        0, 0, 0, 410, 410, 200, -100, -100,
        100, 100, -80, 375, 375, 0, -100, -100,
        100, 150, 50, 250, 250, 80, -150, -150,
        250, 260, 150, -100, -100, -150, -200, -200,
        0, 0, 0, 0, 0, 0, 0, 0,
};

PieceSquareTable g_DEFAULT_PAWN_PST_EG = {
        0, 0, 0, 0, 0, 0, 0, 0,
        550, 550, 550, 550, 550, 550, 550, 550,
        300, 300, 300, 300, 300, 300, 300, 300,
        250, 250, 250, 250, 250, 250, 250, 250,
        100, 100, 100, 100, 100, 100, 100, 100,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
};

PieceSquareTable g_DEFAULT_KING_PST_MG = {
        0, 0, 0, 0, 0, 0, 0, 0,
        -300, -300, -300, -300, -300, -300, -300, -300,
        -500, -500, -500, -500, -500, -500, -500, -500,
        -700, -700, -700, -700, -700, -700, -700, -700,
        -700, -700, -700, -700, -700, -700, -700, -700,
        -500, -500, -500, -500, -500, -500, -500, -500,
        -300, -300, -300, -300, -300, -300, -300, -300,
        100, 100, 50, -300, -300, -100, 350, 350,
};

PieceSquareTable g_DEFAULT_KING_PST_EG = {
        -100, -100, -100, -100, -100, -100, -100, -100,
        -100, 50, 50, 50, 50, 50, 50, -100,
        -100, 50, 125, 125, 125, 125, 50, -100,
        -100, 50, 125, 275, 275, 125, 50, -100,
        -100, 50, 125, 275, 275, 125, 50, -100,
        -100, 50, 125, 125, 125, 125, 50, -100,
        -100, 50, 50, 50, 50, 50, 50, -100,
        -100, -100, -100, -100, -100, -100, -100, -100,
};

PieceSquareTable g_DEFAULT_QUEEN_PST_MG = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, -180, -180, -180, -180, -180, -180, 0,
        0, -180, -400, -400, -400, -400, -180, 0,
        0, -180, -400, -400, -400, -400, -180, 0,
        0, -180, -150, -250, -250, 0, -150, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
};

PieceSquareTable g_DEFAULT_QUEEN_PST_EG = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
};

static HCEWeightTable s_DefaultTable;

void initializeDefaultHCEWeights() {
    auto& tbl = s_DefaultTable;

    tbl.material = {
        HCEWeight(0, 0),        // PT_NONE
        HCEWeight(1000, 1000),  // PT_PAWN
        HCEWeight(3200, 3800),  // PT_KNIGHT
        HCEWeight(3600, 4200),  // PT_BISHOP
        HCEWeight(5100, 6100),  // PT_ROOK
        HCEWeight(10000, 12000), // PT_QUEEN
        HCEWeight(0, 0),        // PT_KING
    };

    tbl.knightMobilityScore = {
            HCEWeight(-380, -330),
            HCEWeight(-250, -230),
            HCEWeight(-120, -130),
            HCEWeight(0, -30),
            HCEWeight(120, 70),
            HCEWeight(250, 170),
            HCEWeight(310, 220),
            HCEWeight(380, 270),
    };

    tbl.bishopMobilityScore = {
            HCEWeight(-250, -30),
            HCEWeight(-110, -16),
            HCEWeight(30, -2),
            HCEWeight(170, 12),
            HCEWeight(310, 26),
            HCEWeight(450, 40),
            HCEWeight(570, 52),
            HCEWeight(650, 60),
            HCEWeight(710, 65),
            HCEWeight(740, 69),
            HCEWeight(760, 71),
            HCEWeight(780, 73),
            HCEWeight(790, 74),
            HCEWeight(800, 75),
            HCEWeight(810, 76),
    };

    tbl.rookHorizontalMobilityScore = {
            HCEWeight(0, 0),
            HCEWeight(0, 0),
            HCEWeight(40, 0),
            HCEWeight(100, 100),
            HCEWeight(100, 100),
            HCEWeight(100, 100),
            HCEWeight(100, 100),
    };

    tbl.rookVerticalMobilityScore = {
            HCEWeight(-100, -200),
            HCEWeight(-50, -100),
            HCEWeight(0, 0),
            HCEWeight(50, 250),
            HCEWeight(100, 400),
            HCEWeight(150, 500),
            HCEWeight(200, 600),
    };

    tbl.kingPstMg = g_DEFAULT_KING_PST_MG;

    tbl.kingPstEg = g_DEFAULT_KING_PST_EG;

    tbl.queenPstMg = g_DEFAULT_QUEEN_PST_MG;

    tbl.queenPstEg = g_DEFAULT_QUEEN_PST_EG;

    tbl.pawnPstEg = g_DEFAULT_PAWN_PST_EG;

    tbl.pawnPstsMg = {
        g_DEFAULT_PAWN_PST_MG_KK,
        g_DEFAULT_PAWN_PST_MG_KQ,
        g_DEFAULT_PAWN_PST_MG_QK,
        g_DEFAULT_PAWN_PST_MG_QQ,
    };

    tbl.knightOutpostScore = { 300, 200 };

    tbl.blockingPawnsScore = { -50, -120 };

    tbl.backwardPawnScore = { -75, -150 };

    tbl.isolatedPawnScore = { -50, -120 };

    tbl.passedPawnScore = {
            HCEWeight(100, 1200),
            HCEWeight(100, 900),
            HCEWeight(100, 750),
            HCEWeight(100, 600),
            HCEWeight(100, 500),
    };

    tbl.kingPawnDistanceScore = { 0, -70 };

    tbl.bishopPairScore = { 150, 260 };

    tbl.rookOnOpenFile = { 200, 400 };

    tbl.rookBehindPasser = { 100, 250 };

    tbl.kingAttackScore =
    {
        0,
        10,
        19,
        28,
        38,
        47,
        57,
        67,
        78,
        88,
        99,
        111,
        122,
        134,
        147,
        160,
        174,
        189,
        204,
        220,
        238,
        256,
        276,
        297,
        319,
        343,
        370,
        398,
        429,
        462,
        499,
        539,
        583,
        631,
        684,
        743,
        807,
        879,
        958,
        1045,
        1142,
        1250,
        1370,
        1503,
        1651,
        1816,
        1999,
        2204,
        2433,
        2687,
        2972,
        3290,
        3644,
        4041,
        4484,
        4500,
        4500,
        4500,
        4500,
        4500,
    };
}

const HCEWeightTable* getDefaultHCEWeights() {
    return &s_DefaultTable;
}

}