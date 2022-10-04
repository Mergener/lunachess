#include "classicevaluator.h"

#include "../../strutils.h"
#include "../../posutils.h"

#include "aibitboards.h"

#include <fstream>

namespace lunachess::ai {

static int adjustScores(int mg, int eg, int gpf) {
    return (mg * gpf) / 100 + (eg * (100 - gpf)) / 100;
}

ScoreTable ClassicEvaluator::defaultMgTable;
ScoreTable ClassicEvaluator::defaultEgTable;

void ClassicEvaluator::generateNewMgTable() {
    // Material values
    defaultMgTable.materialScore[PT_PAWN] = 1000;
    defaultMgTable.materialScore[PT_KNIGHT] = 3300;
    defaultMgTable.materialScore[PT_BISHOP] = 3300;
    defaultMgTable.materialScore[PT_ROOK] = 5100;
    defaultMgTable.materialScore[PT_QUEEN] = 9800;

    // X-Rays
    defaultMgTable.xrayScores[PT_PAWN] = -30;
    defaultMgTable.xrayScores[PT_KNIGHT] = 8;
    defaultMgTable.xrayScores[PT_BISHOP] = 8;
    defaultMgTable.xrayScores[PT_ROOK]   = 48;
    defaultMgTable.xrayScores[PT_QUEEN]  = 96;
    defaultMgTable.xrayScores[PT_KING]   = 101;

    // Mobility scores
    defaultMgTable.mobilityScores[PT_KNIGHT] = 14;
    defaultMgTable.mobilityScores[PT_BISHOP] = 15;
    defaultMgTable.mobilityScores[PT_ROOK] = 12;
    defaultMgTable.mobilityScores[PT_QUEEN] = 4;

    // Piece specific scores
    defaultMgTable.bishopPairScore = 150;
    defaultMgTable.outpostScore = 200;
    defaultMgTable.goodComplexScore = 20;

    // King safety scores
    defaultMgTable.nearKingAttacksScore[PT_PAWN]   = -60;
    defaultMgTable.nearKingAttacksScore[PT_KNIGHT] = -70;
    defaultMgTable.nearKingAttacksScore[PT_BISHOP] = -55;
    defaultMgTable.nearKingAttacksScore[PT_ROOK]   = -100;
    defaultMgTable.nearKingAttacksScore[PT_QUEEN]  = -95;

    defaultMgTable.pawnShieldScore = 90;

    // Pawn structures
    defaultMgTable.blockingPawnScore = -100;
    defaultMgTable.supportPawnsHotmap = Hotmap::defaultMgChainedPawnHotmap;
    defaultMgTable.passersHotmap = Hotmap::defaultMgPasserHotmap;
    defaultMgTable.connectedPassersHotmap = Hotmap::defaultMgConnectedPasserHotmap;

    // Piece activity hotmaps
    for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
        defaultMgTable.getHotmap(pt) = Hotmap::defaultMiddlegameMaps[pt];
    }
    defaultMgTable.kingHotmap = Hotmap::defaultKingMgHotmap;

}

void ClassicEvaluator::generateNewEgTable() {
    // Material values
    defaultEgTable.materialScore[PT_PAWN] = 1300;
    defaultEgTable.materialScore[PT_KNIGHT] = 4200;
    defaultEgTable.materialScore[PT_BISHOP] = 4700;
    defaultEgTable.materialScore[PT_ROOK] = 6700;
    defaultEgTable.materialScore[PT_QUEEN] = 12500;

    // X-Rays
    defaultEgTable.xrayScores[PT_PAWN] = 0;
    defaultEgTable.xrayScores[PT_KNIGHT] = 0;
    defaultEgTable.xrayScores[PT_BISHOP] = 0;
    defaultEgTable.xrayScores[PT_ROOK] = 0;
    defaultEgTable.xrayScores[PT_QUEEN] = 0;
    defaultEgTable.xrayScores[PT_KING] = 0;

    // Mobility scores
    defaultEgTable.mobilityScores[PT_KNIGHT] = 6;
    defaultEgTable.mobilityScores[PT_BISHOP] = 5;
    defaultEgTable.mobilityScores[PT_ROOK] = 8;
    defaultEgTable.mobilityScores[PT_QUEEN] = 4;

    // Piece specific scores
    defaultEgTable.bishopPairScore = 450;
    defaultEgTable.outpostScore = 300;
    defaultEgTable.goodComplexScore = 0;

    // King safety scores
    defaultEgTable.nearKingAttacksScore[PT_PAWN] = -30;
    defaultEgTable.nearKingAttacksScore[PT_KNIGHT] = -40;
    defaultEgTable.nearKingAttacksScore[PT_BISHOP] = -25;
    defaultEgTable.nearKingAttacksScore[PT_ROOK] = -40;
    defaultEgTable.nearKingAttacksScore[PT_QUEEN] = -45;

    defaultEgTable.pawnShieldScore = 15;

    // Pawn structures
    defaultEgTable.blockingPawnScore = -250;
    defaultEgTable.supportPawnsHotmap = Hotmap::defaultEgChainedPawnHotmap;
    defaultEgTable.passersHotmap = Hotmap::defaultEgPasserHotmap;
    defaultEgTable.connectedPassersHotmap = Hotmap::defaultEgConnectedPasserHotmap;

    for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
        defaultEgTable.getHotmap(pt) = Hotmap::defaultEndgameMaps[pt];
    }
    defaultEgTable.kingHotmap = Hotmap::defaultKingEgHotmap;
}

void ClassicEvaluator::initialize() {
    generateNewMgTable();
    generateNewEgTable();
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

    for (PieceType pt = PT_KNIGHT; pt <= PT_QUEEN; ++pt) {
        int mobilityScore = adjustScores(m_MgScores.mobilityScores[pt], m_EgScores.mobilityScores[pt], gpf);

        total += pos.getAttacks(c, pt).count() * mobilityScore;
    }

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

int ClassicEvaluator::evaluatePlacement(const Position& pos, Color c, int gpf) const {
    int total = 0;

    Square ourKingSquare = pos.getKingSquare(c);
    Square theirKingSquare = pos.getKingSquare(getOppositeColor(c));

    for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
        Bitboard bb = pos.getBitboard(Piece(c, pt));
        const Hotmap& hotmapMg = m_MgScores.hotmapGroups[pt - 1].getHotmap(theirKingSquare);
        const Hotmap& hotmapEg = m_EgScores.hotmapGroups[pt - 1].getHotmap(theirKingSquare);

        for (Square s: bb) {
            int score = adjustScores(hotmapMg.getValue(s, c),
                                     hotmapEg.getValue(s, c),
                                     gpf);

            total += score;
        }
    }

    total += adjustScores(m_MgScores.kingHotmap.getValue(ourKingSquare, c),
                          m_EgScores.kingHotmap.getValue(ourKingSquare, c),
                          gpf);

    return total;
}

Bitboard ClassicEvaluator::getPassedPawns(const Position& pos, Color c) {
    Bitboard pawns = pos.getBitboard(Piece(c, PT_PAWN));
    Bitboard theirPawns = pos.getBitboard(Piece(getOppositeColor(c), PT_PAWN));
    Bitboard bb = 0;

    for (Square s: pawns) {
        Bitboard cantHaveEnemyPawnsBB =
                getFileContestantsBitboard(s, c) | getPasserBlockerBitboard(s, c);

        if ((cantHaveEnemyPawnsBB & theirPawns) == 0) {
            // No nearby file contestants and opposing blockers.
            // Pawn is a passer!
            bb.add(s);
        }
    }

    return bb;
}

Bitboard ClassicEvaluator::getChainPawns(const Position& pos, Color c) {
    Bitboard pawns = pos.getBitboard(Piece(c, PT_PAWN));
    Bitboard bb = 0;

    for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
        Bitboard adjacentPawns = 0;
        if (f > FL_A) {
            adjacentPawns |= bbs::getFileBitboard(f - 1) & pawns;
        }
        if (f < FL_COUNT - 1) {
            adjacentPawns |= bbs::getFileBitboard(f + 1) & pawns;
        }
        if (adjacentPawns == 0) {
            // Isolated pawn
            continue;
        }
        bb |= bbs::getFileBitboard(f) & pawns;
    }

    return bb;
}

int ClassicEvaluator::evaluateChainsAndPassers(const Position &pos, Color c, int gpf) const {
    Bitboard pawns = pos.getBitboard(Piece(c, PT_PAWN));
    Bitboard chainedPawns = getChainPawns(pos, c);
    Bitboard passedPawns = getPassedPawns(pos, c);
    Bitboard connectedPassers = chainedPawns & passedPawns;

    int total = 0;

    for (Square s: pawns) {
        if (chainedPawns.contains(s)) {
            total += adjustScores(m_MgScores.supportPawnsHotmap.getValue(s, c),
                                  m_EgScores.supportPawnsHotmap.getValue(s, c),
                                  gpf);
        }
        if (passedPawns.contains(s)) {
            total += adjustScores(m_MgScores.passersHotmap.getValue(s, c),
                                  m_EgScores.passersHotmap.getValue(s, c),
                                  gpf);
        }
        if (connectedPassers.contains(s)) {
            total += adjustScores(m_MgScores.connectedPassersHotmap.getValue(s, c),
                                  m_EgScores.connectedPassersHotmap.getValue(s, c),
                                  gpf);
        }
    }

    return total;
}

int ClassicEvaluator::evaluateBlockingPawns(const Position &pos, Color c, int gpf) const {
    Bitboard pawns = pos.getBitboard(Piece(c, PT_PAWN));

    // Evaluate blocking pawns
    int total = 0;
    int individualBlockerScore = adjustScores(m_MgScores.blockingPawnScore, m_EgScores.blockingPawnScore, gpf);
    for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
        Bitboard filePawns = bbs::getFileBitboard(f) & pawns;
        if (filePawns == 0) {
            continue;
        }

        total += (filePawns.count() - 1) * individualBlockerScore;
    }

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

    return evaluateMaterial(pos, us, gpf) - evaluateMaterial(pos, them, gpf);
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
    int blockingPawns = evaluateBlockingPawns(pos, us, gpf) - evaluateBlockingPawns(pos, them, gpf);
    //int chainsAndPassers = evaluateChainsAndPassers(pos, us, gpf) - evaluateChainsAndPassers(pos, them, gpf);
    int pawnComplex = evaluatePawnComplex(pos, us, gpf) - evaluatePawnComplex(pos, them, gpf);

    // Activity
    int placement = evaluatePlacement(pos, us, gpf) - evaluatePlacement(pos, them, gpf);
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
                + blockingPawns + pawnComplex
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