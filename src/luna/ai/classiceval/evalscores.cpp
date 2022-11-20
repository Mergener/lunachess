#include "evalscores.h"

#include <iomanip>

#include "../../bitboard.h"

namespace lunachess::ai {

//
// Default evaluation parameters declarations
//

static Hotmap defaultPawnMgHotmap;
static Hotmap defaultPawnEgHotmap;
static Hotmap defaultKingEgHotmap;
static Hotmap defaultKingMgHotmap;
static Hotmap defaultMgPasserHotmap;
static Hotmap defaultMgChainedPawnHotmap;
static Hotmap defaultMgConnectedPasserHotmap;
static Hotmap defaultEgPasserHotmap;
static Hotmap defaultEgChainedPawnHotmap;
static Hotmap defaultEgConnectedPasserHotmap;
static ScoreTable defaultMgTable;
static ScoreTable defaultEgTable;

//
// Default evaluation parameters' values
//

static void initializeDefaultMgTable() {
    // Material values
    defaultMgTable.materialScore[PT_PAWN] = 1000;
    defaultMgTable.materialScore[PT_KNIGHT] = 3300;
    defaultMgTable.materialScore[PT_BISHOP] = 3300;
    defaultMgTable.materialScore[PT_ROOK] = 5100;
    defaultMgTable.materialScore[PT_QUEEN] = 9800;

    // Mobility scores
    //                                            0     1     2     3     4     5     6     7     8     9     10     11
    defaultMgTable.mobilityScores[PT_KNIGHT] = {  -80, -80, -80, -80, -60,  0,    0,    60,   60,   60,   60,    60 };
    defaultMgTable.mobilityScores[PT_BISHOP] = {  -120, -120, -115,  -110,  -100,  -65,    -10,   30,  80,  110,  125,   140 };
    defaultMgTable.mobilityScores[PT_ROOK]   = {  -120, -120, -120, -120, -100, -50,  0,    40,   80,   100,  120,   120 };
    defaultMgTable.mobilityScores[PT_QUEEN]  = {  0,    0,    0,    0,    0,    0,    12,   17,   26,   36,   38,   44 };

    // Piece specific scores
    defaultMgTable.bishopPairScore = 180;
    defaultMgTable.outpostScore = 220;
    defaultMgTable.goodComplexScore = 16;

    // King safety scores
    defaultMgTable.nearKingAttacksScore[PT_PAWN]   = 60;
    defaultMgTable.nearKingAttacksScore[PT_KNIGHT] = 60;
    defaultMgTable.nearKingAttacksScore[PT_BISHOP] = 45;
    defaultMgTable.nearKingAttacksScore[PT_ROOK]   = 85;
    defaultMgTable.nearKingAttacksScore[PT_QUEEN]  = 40;

    defaultMgTable.kingExposureScores[PT_BISHOP] = 30;
    defaultMgTable.kingExposureScores[PT_ROOK]   = 40;
    defaultMgTable.kingExposureScores[PT_QUEEN]  = 70;

    // Pawn structures
    defaultMgTable.blockingPawnScore = -80;
    defaultMgTable.chainedPawnsHotmap = defaultMgChainedPawnHotmap;
    defaultMgTable.passersHotmap = defaultMgPasserHotmap;
    defaultMgTable.connectedPassersHotmap = defaultMgConnectedPasserHotmap;

    defaultMgTable.kingHotmap = defaultKingMgHotmap;
    defaultMgTable.pawnsHotmap = defaultPawnMgHotmap;
}

static void initializeDefaultEgTable() {
    // Material values
    defaultEgTable.materialScore[PT_PAWN] = 1300;
    defaultEgTable.materialScore[PT_KNIGHT] = 4200;
    defaultEgTable.materialScore[PT_BISHOP] = 4700;
    defaultEgTable.materialScore[PT_ROOK] = 6700;
    defaultEgTable.materialScore[PT_QUEEN] = 12500;

    // Piece specific scores
    defaultEgTable.bishopPairScore = 450;
    defaultEgTable.outpostScore = 300;
    defaultEgTable.goodComplexScore = 0;

    // King safety scores
    defaultEgTable.nearKingAttacksScore[PT_PAWN] = 30;
    defaultEgTable.nearKingAttacksScore[PT_KNIGHT] = 40;
    defaultEgTable.nearKingAttacksScore[PT_BISHOP] = 25;
    defaultEgTable.nearKingAttacksScore[PT_ROOK] = 40;
    defaultEgTable.nearKingAttacksScore[PT_QUEEN] = 45;

    // Pawn structures
    defaultEgTable.blockingPawnScore = -250;
    defaultEgTable.chainedPawnsHotmap = defaultEgChainedPawnHotmap;
    defaultEgTable.passersHotmap = defaultEgPasserHotmap;
    defaultEgTable.connectedPassersHotmap = defaultEgConnectedPasserHotmap;

    defaultEgTable.kingHotmap = defaultKingEgHotmap;
    defaultEgTable.pawnsHotmap = defaultPawnEgHotmap;
}

std::ostream& operator<<(std::ostream& stream, const Hotmap& hotmap) {
    stream << "      ";
    for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
        stream << getFileIdentifier(f) << "     ";
    }
    for (BoardRank r = RANK_8; r >= RANK_1; --r) {
        stream << std::endl;
        stream << getRankIdentifier(r) << " |";
        for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
            int valueAt = hotmap.valueAt(getSquare(f, r), CL_WHITE);
            std::cout << std::setw(6)
                << std::fixed
                << std::setprecision(2)
                << double(valueAt) / 1000;
        }
    }

    return stream;
}

static void initializeHotmaps() {
    //
    // Pawns
    //
    defaultPawnMgHotmap = {
        0, 0, 0, 0, 0, 0, 0, 0,
        400, 400, 400, 400, 400, 400, 400, 400,
        150,150,150,280,280,150,150,150,
        0, 0, 175, 280, 280, 0, 0, 0,
        0, 0, 175, 280, 280, 0, 0, 0,
        0, 0, 100, 120, 120, 0, 0, 0,
        0, 0, -20, -20, -20, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    };

    defaultPawnEgHotmap = {
        0, 0, 0, 0, 0, 0, 0, 0,
        160, 160, 160, 160, 160, 160, 160, 160,
        132, 132, 132, 132, 132, 132, 132, 132,
        104, 104, 104, 104, 104, 104, 104, 104,
        76, 76, 76, 76, 76, 76, 76, 76,
        48, 48, 48, 48, 48, 48, 48, 48,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
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
        50, 50, 50, 50, 50, 50, 50, 50,
        45, 45, 45, 45, 45, 45, 45, 45,
        40, 40, 40, 40, 40, 40, 40, 40,
        35, 35, 35, 35, 35, 35, 35, 35,
        30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30,
        0, 0, 0, 0, 0, 0, 0, 0,
    };

    defaultEgPasserHotmap = {
        0, 0, 0, 0, 0, 0, 0, 0,
        70, 70, 70, 70, 70, 70, 70, 70,
        65, 65, 65, 65, 65, 65, 65, 65,
        60, 60, 60, 60, 60, 60, 60, 60,
        55, 55, 55, 55, 55, 55, 55, 55,
        50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50,
        0, 0, 0, 0, 0, 0, 0, 0,
    };

    defaultMgChainedPawnHotmap = {
        0, 0, 0, 0, 0, 0, 0, 0,
        50, 50, 50, 50, 50, 50, 50, 50,
        46, 46, 46, 46, 46, 46, 46, 46,
        42, 42, 42, 42, 42, 42, 42, 42,
        38, 38, 38, 38, 38, 38, 38, 38,
        34, 34, 34, 34, 34, 34, 34, 34,
        30, 30, 30, 30, 30, 30, 30, 30,
        0, 0, 0, 0, 0, 0, 0, 0,
    };

    defaultEgChainedPawnHotmap = {
        0, 0, 0, 0, 0, 0, 0, 0,
        80, 80, 80, 80, 80, 80, 80, 80,
        74, 74, 74, 74, 74, 74, 74, 74,
        68, 68, 68, 68, 68, 68, 68, 68,
        62, 62, 62, 62, 62, 62, 62, 62,
        56, 56, 56, 56, 56, 56, 56, 56,
        50, 50, 50, 50, 50, 50, 50, 50,
        0, 0, 0, 0, 0, 0, 0, 0,
    };
//
//    defaultMgConnectedPasserHotmap = {
//            0, 0, 0, 0, 0, 0, 0, 0,
//            50, 50, 50, 50, 50, 50, 50, 50,
//            46, 46, 46, 46, 46, 46, 46, 46,
//            42, 42, 42, 42, 42, 42, 42, 42,
//            38, 38, 38, 38, 38, 38, 38, 38,
//            34, 34, 34, 34, 34, 34, 34, 34,
//            30, 30, 30, 30, 30, 30, 30, 30,
//            0, 0, 0, 0, 0, 0, 0, 0,
//    };
//
//    defaultEgConnectedPasserHotmap = {
//            0, 0, 0, 0, 0, 0, 0, 0,
//            80, 80, 80, 80, 80, 80, 80, 80,
//            74, 74, 74, 74, 74, 74, 74, 74,
//            68, 68, 68, 68, 68, 68, 68, 68,
//            62, 62, 62, 62, 62, 62, 62, 62,
//            56, 56, 56, 56, 56, 56, 56, 56,
//            50, 50, 50, 50, 50, 50, 50, 50,
//            0, 0, 0, 0, 0, 0, 0, 0,
//    };
}

void initializeEvalScores() {
    initializeHotmaps();
    initializeDefaultMgTable();
    initializeDefaultEgTable();
}

const ScoreTable& ScoreTable::getDefaultMiddlegameTable() {
    return defaultMgTable;
}

const ScoreTable& ScoreTable::getDefaultEndgameTable() {
    return defaultEgTable;
}

} // lunachess::ai