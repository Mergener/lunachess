#include "classicevaluator.h"

#include "../strutils.h"
#include "../posutils.h"

#include "aibitboards.h"

#include <optional>
#include <fstream>

namespace lunachess::ai {

static int adjustScores(int mg, int eg, int gpf) {
    return (mg * gpf) / 100 + (eg * (100 - gpf)) / 100;
}

ScoreTable ClassicEvaluator::defaultMgTable;
ScoreTable ClassicEvaluator::defaultEgTable;

void ClassicEvaluator::generateNewMgTable() {
    defaultMgTable.materialScore[PT_PAWN] = 1000;
    defaultMgTable.materialScore[PT_KNIGHT] = 3000;
    defaultMgTable.materialScore[PT_BISHOP] = 3100;
    defaultMgTable.materialScore[PT_ROOK] = 5000;
    defaultMgTable.materialScore[PT_QUEEN] = 9200;
    defaultMgTable.materialScore[PT_KING] = 0;

    defaultMgTable.kingOnOpenFileScore = -120;
    defaultMgTable.kingOnSemiOpenFileScore = -80;
    defaultMgTable.kingNearOpenFileScore = -100;
    defaultMgTable.kingNearSemiOpenFileScore = -60;
    defaultMgTable.nearKingSquareAttacksScore = -25;

    defaultMgTable.pawnShieldScore = 120;

    defaultMgTable.tropismScore[PT_PAWN] = 300;
    defaultMgTable.tropismScore[PT_KNIGHT] = 360;
    defaultMgTable.tropismScore[PT_BISHOP] = 140;
    defaultMgTable.tropismScore[PT_ROOK] = 300;
    defaultMgTable.tropismScore[PT_QUEEN] = 240;
    defaultMgTable.tropismScore[PT_KING] = 0;

    defaultMgTable.xrayScores[PT_PAWN] = -12;
    defaultMgTable.xrayScores[PT_KNIGHT] = 10;
    defaultMgTable.xrayScores[PT_BISHOP] = 10;
    defaultMgTable.xrayScores[PT_ROOK] = 30;
    defaultMgTable.xrayScores[PT_QUEEN] = 50;
    defaultMgTable.xrayScores[PT_KING] = 60;

    defaultMgTable.mobilityScore = 30;

    defaultMgTable.goodComplexScore = 50;

    defaultMgTable.doublePawnScore = -100;

    defaultMgTable.outpostScore = 160;

    defaultMgTable.bishopPairScore = 120;

    defaultMgTable.pawnChainScore = 60;

    defaultMgTable.passerPercentBonus = 40;
    defaultMgTable.outsidePasserPercentBonus = 60;

    for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
        defaultMgTable.getHotmap(pt) = Hotmap::defaultMiddlegameMaps[pt];
    }
    defaultMgTable.kingHotmap = Hotmap::defaultKingMgHotmap;
}

void ClassicEvaluator::generateNewEgTable() {
    defaultEgTable.materialScore[PT_PAWN] = 1000;
    defaultEgTable.materialScore[PT_KNIGHT] = 3900;
    defaultEgTable.materialScore[PT_BISHOP] = 4400;
    defaultEgTable.materialScore[PT_ROOK] = 6900;
    defaultEgTable.materialScore[PT_QUEEN] = 11200;
    defaultEgTable.materialScore[PT_KING] = 0;

    defaultEgTable.xrayScores[PT_PAWN] = 0;
    defaultEgTable.xrayScores[PT_KNIGHT] = 0;
    defaultEgTable.xrayScores[PT_BISHOP] = 0;
    defaultEgTable.xrayScores[PT_ROOK] = 0;
    defaultEgTable.xrayScores[PT_QUEEN] = 0;
    defaultEgTable.xrayScores[PT_KING] = 0;

    defaultEgTable.pawnShieldScore = 15;

    defaultEgTable.mobilityScore = 4;

    defaultEgTable.doublePawnScore = -250;

    defaultEgTable.outpostScore = 300;

    defaultEgTable.bishopPairScore = 400;

    defaultEgTable.tropismScore[PT_PAWN] = 0;
    defaultEgTable.tropismScore[PT_BISHOP] = 40;
    defaultEgTable.tropismScore[PT_KNIGHT] = 120;
    defaultEgTable.tropismScore[PT_ROOK] = 90;
    defaultEgTable.tropismScore[PT_QUEEN] = 10;
    defaultEgTable.tropismScore[PT_KING] = 400;

    defaultEgTable.kingOnOpenFileScore = 0;
    defaultEgTable.kingOnSemiOpenFileScore = 0;
    defaultEgTable.kingNearOpenFileScore = 0;
    defaultEgTable.kingNearSemiOpenFileScore = 0;
    defaultEgTable.nearKingSquareAttacksScore = -30;

    defaultEgTable.pawnChainScore = 180;

    defaultEgTable.passerPercentBonus = 50;
    defaultEgTable.outsidePasserPercentBonus = 80;

    for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
        defaultEgTable.getHotmap(pt) = Hotmap::defaultEndgameMaps[pt];
    }
    defaultEgTable.kingHotmap = Hotmap::defaultKingEgHotmap;
}

void ClassicEvaluator::initialize() {
    generateNewMgTable();
    generateNewEgTable();
}

ClassicEvaluator::PasserData ClassicEvaluator::getPasserData(const Position& pos, Color c, int gpf) const {
    PasserData ret;

    Color them = getOppositeColor(c);
    Bitboard ourPawns = pos.getBitboard(Piece(c, PT_PAWN));
    Bitboard theirPawns = pos.getBitboard(Piece(them, PT_PAWN));

    for (Square s: ourPawns) {
        Bitboard blockerBB = getPasserBlockerBitboard(s, c) & theirPawns;

        if (blockerBB == 0) {
            // No opposing blockers, we have a passer!
            ret.allPassers.add(s);

            // Now, check if it is an outside passer.
            Bitboard contestantBB = getFileContestantsBitboard(s, c) & theirPawns;
            if (contestantBB == 0) {
                // It is an outside passer!
                ret.outsidePassers.add(s);
            }
        }
    }

    ret.passerPercentBonus = adjustScores(m_MgScores.passerPercentBonus, m_EgScores.passerPercentBonus, gpf);
    ret.outsidePasserPercentBonus = adjustScores(m_MgScores.outsidePasserPercentBonus, m_EgScores.outsidePasserPercentBonus, gpf);

    return ret;
}

int ClassicEvaluator::evaluateMaterial(const Position& pos, Color c, int gpf) const {
    int total = 0;

    for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
        Bitboard bb = pos.getBitboard(Piece(c, pt));

        int materialScore = adjustScores(m_MgScores.materialScore[pt],
                                         m_EgScores.materialScore[pt],
                                         gpf);

        total += bb.count() * materialScore;
    }

    return total;
}

int ClassicEvaluator::evaluatePawnComplex(const Position& pos, Color color, int gpf) const {
    // Score pawns that are on squares that favor the movement of a color's bishop.

    Bitboard lightSquares = bbs::LIGHT_SQUARES;
    Bitboard darkSquares = bbs::DARK_SQUARES;

    Bitboard lightSquaredBishops = pos.getBitboard(Piece(color, PT_BISHOP)) & lightSquares;
    Bitboard darkSquaredBishops = pos.getBitboard(Piece(color, PT_BISHOP)) & darkSquares;

    Bitboard desiredColorComplex;
    int nlsb = lightSquaredBishops.count();
    int ndsb = darkSquaredBishops.count();
    if (nlsb == ndsb) {
        return 0;
    }
    if (ndsb > nlsb) {
        // More dark squared bishops, we want more pawns in light squares
        desiredColorComplex = lightSquares;
    }
    else {
        // More light squared bishops, we want more pawns in dark squares
        desiredColorComplex = darkSquares;
    }

    int individualScore = adjustScores(m_MgScores.goodComplexScore, m_EgScores.goodComplexScore, gpf);

    Bitboard pawns = pos.getBitboard(Piece(PT_PAWN, color)) & desiredColorComplex;

    return pawns.count() * individualScore;
}

int ClassicEvaluator::evaluateBlockingPawns(const Position& pos, Color c, int gpf) const {
    int total = 0;

    int doublePawnScore = adjustScores(m_MgScores.doublePawnScore, m_EgScores.doublePawnScore, gpf);

    Bitboard pawns = pos.getBitboard(Piece(c, PT_PAWN));

    for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
        Bitboard fileBB = bbs::getFileBitboard(f);

        Bitboard filePawns = fileBB & pawns;

        // If two or more pawns of the same color are in a file,
        // all but one are 'blocking' pawns.
        int blockers = filePawns.count() - 1;
        if (blockers > 0) {
            total += blockers * doublePawnScore;
        }
    }

    return total;
}

int ClassicEvaluator::evaluateOutposts(const Position& pos, Color color, int gpf) const {
    int total = 0;
    int individualScore = adjustScores(m_MgScores.outpostScore, m_EgScores.outpostScore, gpf);

    Bitboard knightBB = pos.getBitboard(Piece(color, PT_KNIGHT));
    Bitboard opponentPawnBB = pos.getBitboard(Piece(getOppositeColor(color), PT_PAWN));

    for (auto sq : knightBB) {
        Bitboard contestantBB = getFileContestantsBitboard(sq, color);

        // This knight is in an outpost if there are no opponent pawns within the contestant bitboard.
        contestantBB = contestantBB & opponentPawnBB;
        if (contestantBB.count() == 0) {
            total += individualScore;
        }
    }

    return total;
}

int ClassicEvaluator::evaluateTropism(const Position& pos, Color c, int gpf) const {
    int total = 0;

    Square kingSquare = pos.getKingSquare(c);

    Color them = getOppositeColor(c);

    bool weHaveMoreMaterial = pos.countMaterial(c) > pos.countMaterial(them);

    for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
        if (weHaveMoreMaterial && pt == PT_KING) {
            // Only give penalty for king proximity if they have more material
            // than us
            continue;
        }

        int trop = adjustScores(m_MgScores.tropismScore[pt], m_EgScores.tropismScore[pt], gpf);

        Bitboard bb = pos.getBitboard(Piece(them, pt));
        for (auto s : bb) {
            // For every opponent piece, count tropism score based on their
            // distance from our king
            int dist = getManhattanDistance(kingSquare, s);
            if (dist == 0) {
                dist = 1;
            }
            total -= trop / dist; // The higher the distance, the lower the penalty.
        }
    }
    return total;
}

int ClassicEvaluator::evaluateKingExposure(const Position& pos, Color c, int gpf) const {
    Color them = getOppositeColor(c);
    Bitboard theirFileAttackers = pos.getBitboard(Piece(them, PT_QUEEN)) |
                                  pos.getBitboard(Piece(them, PT_ROOK));

    if (theirFileAttackers == 0) {
        // No problem being on an open file if they have no rooks or queens.
        return 0;
    }

    // They have queens or rook, being on an open/semiopen file is dangerous
    int total = 0;
    Square kingSquare = pos.getKingSquare(c);
    BoardFile kingFile = getFile(kingSquare);

    // Check king's file
    switch (posutils::getFileState(pos, kingFile)) {
        case FS_SEMIOPEN:
            total += adjustScores(m_MgScores.kingOnSemiOpenFileScore, m_EgScores.kingOnSemiOpenFileScore, gpf);
            break;

        case FS_OPEN:
            total += adjustScores(m_MgScores.kingOnOpenFileScore, m_EgScores.kingOnOpenFileScore, gpf);
            break;

        default:
            break;
    }

    // Check adjacent files
    for (int i = 0; i <= 2; i += 2) {
        BoardFile adjacentFile = kingFile - 1 + i;
        if (adjacentFile < 0 || adjacentFile >= FL_COUNT) {
            // Invalid file
            break;
        }

        switch (posutils::getFileState(pos, adjacentFile)) {
            case FS_SEMIOPEN:
                total += adjustScores(m_MgScores.kingNearSemiOpenFileScore, m_EgScores.kingNearSemiOpenFileScore, gpf);
                break;

            case FS_OPEN:
                total += adjustScores(m_MgScores.kingNearOpenFileScore, m_EgScores.kingNearOpenFileScore, gpf);
                break;

            default:
                break;
        }
    }

    return total;
}

int ClassicEvaluator::evaluatePawnChains(const Position& pos, Color c, int gpf, const PasserData& pd) const {
    int total = 0;
    int pawnChainScore = adjustScores(m_MgScores.pawnChainScore, m_EgScores.pawnChainScore, gpf);

    Bitboard pawns = pos.getBitboard(Piece(c, PT_PAWN));
    bool prevHasPawns = pawns & bbs::getFileBitboard(FL_A);

    for (BoardFile f = FL_B; f < FL_COUNT; ++f) {
        Bitboard filePawns = pawns & bbs::getFileBitboard(f);
        bool hasPawn = filePawns != 0;

        if (hasPawn && prevHasPawns) {
            int score = pd.multiplyScore(pawnChainScore,
                                         (filePawns & pd.allPassers) != 0,
                                         (filePawns & pd.outsidePassers) != 0);

            total += score;
        }
        prevHasPawns = hasPawn;
    }

    return total;
}

int ClassicEvaluator::evaluateBishopPair(const Position& pos, Color color, int gpf) const {
    // min(nLightSquaredBishops, nDarkSquaredBishops) gives us the number of bishop pairs.
    // Multiply it by the bishop pair individual score to obtain the bishop pair bonus.

    Bitboard lightSquares = bbs::LIGHT_SQUARES;
    Bitboard darkSquares = bbs::DARK_SQUARES;

    // Get all bishops
    auto bishopBB = pos.getBitboard(Piece(color, PT_BISHOP));

    Bitboard lightSquaredBishops = bishopBB & lightSquares;
    Bitboard darkSquaredBishops = bishopBB & darkSquares;

    int individualScore = adjustScores(m_MgScores.bishopPairScore, m_EgScores.bishopPairScore, gpf);

    return std::min(lightSquaredBishops.count(), darkSquaredBishops.count()) * individualScore;
}

int ClassicEvaluator::evaluateMobility(const Position& pos, Color c, int gpf) const {
    int total = 0;

    int mobilityScore = adjustScores(m_MgScores.mobilityScore, m_EgScores.mobilityScore, gpf);

    Bitboard theirPawnAttacks = pos.getAttacks(getOppositeColor(c), PT_PAWN);

    total += Bitboard(pos.getAttacks(c, PT_BISHOP) & (~theirPawnAttacks)).count() * mobilityScore;
    total += Bitboard(pos.getAttacks(c, PT_ROOK) & (~theirPawnAttacks)).count() * mobilityScore / 2;
    total += Bitboard(pos.getAttacks(c, PT_QUEEN) & (~theirPawnAttacks)).count() * mobilityScore / 5;

    return total;
}

int ClassicEvaluator::evaluateXrays(const Position& pos, Color c, int gpf) const {
    int total = 0;

    Color them = getOppositeColor(c);

    // Generate scores
    int xrays[PT_COUNT];
    for (PieceType pt = PT_PAWN; pt < PT_COUNT; ++pt) {
        xrays[pt] = adjustScores(m_MgScores.xrayScores[pt], m_EgScores.xrayScores[pt], gpf);
    }

    for (PieceType pt = PT_BISHOP; pt <= PT_QUEEN; ++pt) {
        Bitboard bb = pos.getBitboard(Piece(c, pt));

        for (auto s : bb) {
            Bitboard attacks = bbs::getSliderAttacks(s, 0, pt);

            for (PieceType theirPt = PT_PAWN; theirPt < PT_COUNT; ++theirPt) {
                Bitboard theirPieceBB = pos.getBitboard(Piece(them, theirPt));

                total += Bitboard(attacks & theirPieceBB).count() * xrays[theirPt];
            }
        }
    }

    return total;
}

int ClassicEvaluator::evaluatePawnShield(const Position& pos, Color c, int gpf) const {
    int pawnShieldScore = adjustScores(m_MgScores.pawnShieldScore, m_EgScores.pawnShieldScore, gpf);
    Square kingSquare = pos.getKingSquare(c);
    Bitboard pawnShieldBB = getPawnShieldBitboard(kingSquare, c) & pos.getBitboard(Piece(c, PT_PAWN));
    return pawnShieldBB.count() * pawnShieldScore;
}

int ClassicEvaluator::evaluatePlacement(const Position& pos, Color c, int gpf, const PasserData& pd) const {
    int total = 0;

    Square ourKingSquare = pos.getKingSquare(c);
    Square theirKingSquare = pos.getKingSquare(getOppositeColor(c));

    for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
        Bitboard bb = pos.getBitboard(Piece(c, pt));

        for (Square s: bb) {
            const Hotmap& hotmapMg = m_MgScores.hotmapGroups[pt - 1].getHotmap(theirKingSquare);
            const Hotmap& hotmapEg = m_EgScores.hotmapGroups[pt - 1].getHotmap(theirKingSquare);

            int score = adjustScores(hotmapMg.getValue(s, c),
                                     hotmapEg.getValue(s, c),
                                     gpf);

            if (pt == PT_PAWN) {
                score = pd.multiplyScoreIfPasser(s, score);
            }

            total += score;
        }
    }

    total += adjustScores(m_MgScores.kingHotmap.getValue(ourKingSquare, c),
                          m_EgScores.kingHotmap.getValue(ourKingSquare, c),
                          gpf);

    return total;
}

int ClassicEvaluator::getDrawScore() const {
    return 0;
}

int ClassicEvaluator::getGamePhaseFactor(const Position& pos) const {
    constexpr int STARTING_MATERIAL_COUNT = 62; // Excluding pawns

    int totalMaterial = pos.countMaterial<true>();
    int ret = (totalMaterial * 100) / STARTING_MATERIAL_COUNT;

    return ret;
}

int ClassicEvaluator::evaluateShallow(const Position &pos) const {
    int gpf = getGamePhaseFactor(pos);

    Color us = pos.getColorToMove();
    Color them = getOppositeColor(us);

    PasserData ourPasserData = getPasserData(pos, us, gpf);
    PasserData theirPasserData = getPasserData(pos, them, gpf);

    return evaluateMaterial(pos, us, gpf) - evaluateMaterial(pos, them, gpf)
        + evaluatePlacement(pos, us, gpf, ourPasserData) - evaluatePlacement(pos, them, gpf, theirPasserData);
}

int ClassicEvaluator::evaluate(const Position& pos) const {
    int gpf = getGamePhaseFactor(pos);

    Color us = pos.getColorToMove();
    Color them = getOppositeColor(us);

    int material = evaluateMaterial(pos, us, gpf) - evaluateMaterial(pos, them, gpf);

    // Pawn structure
    PasserData ourPasserData = getPasserData(pos, us, gpf);
    PasserData theirPasserData = getPasserData(pos, them, gpf);
    int doublePawns = evaluateBlockingPawns(pos, us, gpf) - evaluateBlockingPawns(pos, them, gpf);
    int pawnChains = evaluatePawnChains(pos, us, gpf, ourPasserData) - evaluatePawnChains(pos, them, gpf, theirPasserData);

    // Activity
    int placement = evaluatePlacement(pos, us, gpf, ourPasserData) - evaluatePlacement(pos, them, gpf, theirPasserData);
    int bishopPair = evaluateBishopPair(pos, us, gpf) - evaluateBishopPair(pos, them, gpf);
    int mobility = evaluateMobility(pos, us, gpf) - evaluateMobility(pos, them, gpf);
    int outposts = evaluateOutposts(pos, us, gpf) - evaluateOutposts(pos, them, gpf);
    int xrays = evaluateXrays(pos, us, gpf) - evaluateXrays(pos, them, gpf);
    
    // King safety
    int tropism = evaluateTropism(pos, us, gpf) - evaluateTropism(pos, them, gpf);
    int pawnShield = evaluatePawnShield(pos, us, gpf) - evaluatePawnShield(pos, them, gpf);
    int kingExposure = evaluateKingExposure(pos, us, gpf) - evaluateKingExposure(pos, them, gpf);

    int total = placement + bishopPair + mobility
            + outposts + xrays
            + doublePawns + pawnChains +
            + tropism + pawnShield + kingExposure
            + material;

    return total;
}

ClassicEvaluator::ClassicEvaluator() {
    m_MgScores = defaultMgTable;
    m_EgScores = defaultEgTable;
}

}