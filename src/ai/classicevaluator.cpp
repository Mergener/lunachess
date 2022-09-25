#include "classicevaluator.h"

#include "../strutils.h"
#include "../posutils.h"

#include "aibitboards.h"

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

    defaultMgTable.pawnShieldScore = 50;

    defaultMgTable.tropismScore[PT_PAWN] = 300;
    defaultMgTable.tropismScore[PT_KNIGHT] = 360;
    defaultMgTable.tropismScore[PT_BISHOP] = 140;
    defaultMgTable.tropismScore[PT_ROOK] = 300;
    defaultMgTable.tropismScore[PT_QUEEN] = 240;
    defaultMgTable.tropismScore[PT_KING] = 0;

    defaultMgTable.xrayScores[PT_PAWN] = -12;
    defaultMgTable.xrayScores[PT_KNIGHT] = 2;
    defaultMgTable.xrayScores[PT_BISHOP] = 3;
    defaultMgTable.xrayScores[PT_ROOK] = 16;
    defaultMgTable.xrayScores[PT_QUEEN] = 60;
    defaultMgTable.xrayScores[PT_KING] = 70;

    defaultMgTable.nearKingAttacksScore[PT_PAWN] = -22;
    defaultMgTable.nearKingAttacksScore[PT_KNIGHT] = -27;
    defaultMgTable.nearKingAttacksScore[PT_BISHOP] = -25;
    defaultMgTable.nearKingAttacksScore[PT_ROOK] = -32;
    defaultMgTable.nearKingAttacksScore[PT_QUEEN] = -38;

    defaultMgTable.mobilityScore = 46;

    defaultMgTable.goodComplexScore = 25;

    defaultMgTable.doublePawnScore = -50;

    defaultMgTable.outpostScore = 188;

    defaultMgTable.bishopPairScore = 90;

    defaultMgTable.pawnChainScore = 45;

    defaultMgTable.passerPercentBonus = 5;
    defaultMgTable.outsidePasserPercentBonus = 70;

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

    defaultEgTable.doublePawnScore = -120;

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

    defaultEgTable.pawnChainScore = 120;

    defaultEgTable.passerPercentBonus = 10;
    defaultEgTable.outsidePasserPercentBonus = 85;

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

int ClassicEvaluator::evaluateNearKingAttacks(const Position& pos, Color c, int gpf) const {
    Color them = getOppositeColor(c);
    Bitboard nearKingBB = getNearKingSquares(pos.getKingSquare(c));

    Bitboard pawnHits = nearKingBB & pos.getAttacks(them, PT_PAWN);
    Bitboard knightHits = nearKingBB & pos.getAttacks(them, PT_KNIGHT);
    Bitboard bishopHits = nearKingBB & pos.getAttacks(them, PT_BISHOP);
    Bitboard rookHits = nearKingBB & pos.getAttacks(them, PT_ROOK);
    Bitboard queenHits = nearKingBB & pos.getAttacks(them, PT_QUEEN);

    int total = 0;

    total += pawnHits.count() * adjustScores(m_MgScores.nearKingAttacksScore[PT_PAWN],
                                             m_EgScores.nearKingAttacksScore[PT_PAWN],
                                             gpf);

    total += knightHits.count() * adjustScores(m_MgScores.nearKingAttacksScore[PT_KNIGHT],
                                             m_EgScores.nearKingAttacksScore[PT_KNIGHT],
                                             gpf);

    total += bishopHits.count() * adjustScores(m_MgScores.nearKingAttacksScore[PT_BISHOP],
                                             m_EgScores.nearKingAttacksScore[PT_BISHOP],
                                             gpf);

    total += rookHits.count() * adjustScores(m_MgScores.nearKingAttacksScore[PT_ROOK],
                                             m_EgScores.nearKingAttacksScore[PT_ROOK],
                                             gpf);

    total += queenHits.count() * adjustScores(m_MgScores.nearKingAttacksScore[PT_QUEEN],
                                             m_EgScores.nearKingAttacksScore[PT_QUEEN],
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
    // First, check if we are facing a known endgame
    EndgameData eg = endgame::identify(pos);
    if (eg.type == EG_UNKNOWN) {
        // Not a known endgame
        return evaluateClassic(pos);
    }
    if (eg.lhs == pos.getColorToMove()) {
        // Evaluate the endgame on our perspective
        return evaluateEndgame(pos, eg);
    }
    // Evaluate the endgame on their perspective
    return -evaluateEndgame(pos, eg);
}

int ClassicEvaluator::evaluateClassic(const Position& pos) const {
    int gpf = getGamePhaseFactor(pos);

    Color us = pos.getColorToMove();
    Color them = getOppositeColor(us);

    int material = evaluateMaterial(pos, us, gpf) - evaluateMaterial(pos, them, gpf);

    // Pawn structure
    PasserData ourPasserData = getPasserData(pos, us, gpf);
    PasserData theirPasserData = getPasserData(pos, them, gpf);
    int doublePawns = evaluateBlockingPawns(pos, us, gpf) - evaluateBlockingPawns(pos, them, gpf);
    int pawnChains = evaluatePawnChains(pos, us, gpf, ourPasserData) - evaluatePawnChains(pos, them, gpf, theirPasserData);
    int pawnComplex = evaluatePawnComplex(pos, us, gpf) - evaluatePawnComplex(pos, them, gpf);

    // Activity
    int placement = evaluatePlacement(pos, us, gpf, ourPasserData) - evaluatePlacement(pos, them, gpf, theirPasserData);
    int bishopPair = evaluateBishopPair(pos, us, gpf) - evaluateBishopPair(pos, them, gpf);
    int mobility = evaluateMobility(pos, us, gpf) - evaluateMobility(pos, them, gpf);
    int outposts = evaluateOutposts(pos, us, gpf) - evaluateOutposts(pos, them, gpf);
    int xrays = evaluateXrays(pos, us, gpf) - evaluateXrays(pos, them, gpf);

    // King safety
    int pawnShield = evaluatePawnShield(pos, us, gpf) - evaluatePawnShield(pos, them, gpf);
    //int kingExposure = evaluateKingExposure(pos, us, gpf) - evaluateKingExposure(pos, them, gpf);
    int nearKingAttacks = evaluateNearKingAttacks(pos, us, gpf) - evaluateNearKingAttacks(pos, them, gpf);

    int total = placement + bishopPair + mobility
                + outposts + xrays
                + doublePawns + pawnChains + pawnComplex
                + pawnShield + nearKingAttacks
                + material;

    return total;
}

int ClassicEvaluator::evaluateEndgame(const Position& pos, EndgameData egData) const {
    switch (egData.type) {
        // Drawn endgames
        case EG_KR_KN:
        case EG_KR_KB:
            return getDrawScore();

        case EG_KP_K:
            return evaluateKPK(pos, egData.lhs);

        case EG_KBN_K:
            return evaluateKBNK(pos, egData.lhs);

        default:
            // Not implemented endgame, resort to default evaluation:
            return evaluateClassic(pos);
    }

}

int ClassicEvaluator::evaluateKPK(const Position &pos, Color lhs) const {
    int queenValue = m_EgScores.materialScore[PT_QUEEN];

    Color rhs = getOppositeColor(lhs);
    Square pawnSquare = *pos.getBitboard(Piece(lhs, PT_PAWN)).begin();
    Square enemyKingSquare = pos.getKingSquare(rhs);

    if (!endgame::isInsideTheSquare(pawnSquare, enemyKingSquare, lhs, pos.getColorToMove())) {
        // King outside of square, pawn can promote freely.
        BoardRank promRank = getPromotionRank(lhs);
        BoardRank pawnRank = getRank(pawnSquare);
        int dist = std::abs(promRank - pawnRank);
        return queenValue - dist * 100;
    }

    return evaluateClassic(pos);
}

int ClassicEvaluator::evaluateKBNK(const Position &pos, Color lhs) const {
    constexpr int LONE_KING_BONUS_DS[] {
        0, 1, 2, 3, 4, 5, 6, 7,
        1, 2, 3, 4, 5, 6, 7, 6,
        2, 3, 4, 5, 6, 7, 6, 5,
        3, 4, 5, 6, 7, 6, 5, 4,
        4, 5, 6, 7, 6, 5, 4, 3,
        5, 6, 7, 6, 5, 4, 3, 2,
        6, 7, 6, 5, 4, 3, 2, 1,
        7, 6, 5, 4, 3, 2, 1, 0,
    };
    constexpr int LONE_KING_BONUS_LS[] {
        7, 6, 5, 4, 3, 2, 1, 0,
        6, 7, 6, 5, 4, 3, 2, 1,
        5, 6, 7, 6, 5, 4, 3, 2,
        4, 5, 6, 7, 6, 5, 4, 3,
        3, 4, 5, 6, 7, 6, 5, 4,
        2, 3, 4, 5, 6, 7, 6, 5,
        1, 2, 3, 4, 5, 6, 7, 6,
        0, 1, 2, 3, 4, 5, 6, 7,
    };

    int base = m_EgScores.materialScore[PT_BISHOP] +
               m_EgScores.materialScore[PT_KNIGHT] +
               m_EgScores.materialScore[PT_PAWN] / 2;

    Square ourBishop = *pos.getBitboard(Piece(lhs, PT_BISHOP)).begin();
    Square theirKing = pos.getKingSquare(getOppositeColor(lhs));

    int theirKingBonus;
    if (bbs::LIGHT_SQUARES.contains(ourBishop)) {
        theirKingBonus = LONE_KING_BONUS_LS[theirKing];
    }
    else {
        theirKingBonus = LONE_KING_BONUS_DS[theirKing];
    }

    return base - theirKingBonus * 50;
}

ClassicEvaluator::ClassicEvaluator() {
    m_MgScores = defaultMgTable;
    m_EgScores = defaultEgTable;
}

}