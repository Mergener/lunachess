#include "classicevaluator.h"

#include "../../strutils.h"
#include "../../staticanalysis.h"

#include <fstream>

namespace lunachess::ai {

static int adjustScores(int mg, int eg, int gpf) {
    return (mg * gpf) / 100 + (eg * (100 - gpf)) / 100;
}

int HandCraftedEvaluator<>::getGamePhaseFactor(const Position& pos) const {
    constexpr int KNIGHT_VAL = 3;
    constexpr int BISHOP_VAL = 3;
    constexpr int ROOK_VAL   = 5;
    constexpr int QUEEN_VAL  = 12;
    constexpr int STARTING_MATERIAL_COUNT =
            KNIGHT_VAL * 2 + BISHOP_VAL * 2 +
            ROOK_VAL * 2   + QUEEN_VAL * 1; // Excluding pawns

    int nKnights = bits::popcount(pos.getBitboard(WHITE_KNIGHT) | pos.getBitboard(BLACK_KNIGHT));
    int nBishops = bits::popcount(pos.getBitboard(WHITE_BISHOP) | pos.getBitboard(BLACK_BISHOP));
    int nRooks   = bits::popcount(pos.getBitboard(WHITE_ROOK) | pos.getBitboard(BLACK_ROOK));
    int nQueens  = bits::popcount(pos.getBitboard(WHITE_QUEEN) | pos.getBitboard(BLACK_QUEEN));

    int total = nKnights * KNIGHT_VAL +
                nBishops * BISHOP_VAL +
                nRooks + ROOK_VAL +
                nQueens + QUEEN_VAL;

    int ret = (total * 100) / STARTING_MATERIAL_COUNT;

    std::cout << "total " << total << " ret " << ret << std::endl;

    return ret;
}

int HandCraftedEvaluator<>::evaluateMaterial(const Position& pos, Color c, int gpf) const {
    if constexpr (!DO_MATERIAL) {
        return 0;
    }

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

int HandCraftedEvaluator<>::evaluatePawnComplex(const Position& pos, Color color, int gpf) const {
    if constexpr (!DO_PAWN_COMPLEX) {
        return 0;
    }

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

    Bitboard pawns = pos.getBitboard(Piece(color, PT_PAWN)) & desiredColorComplex;

    return pawns.count() * individualScore;
}

int HandCraftedEvaluator<>::evaluateOutposts(const Position& pos, Color color, int gpf) const {
    if constexpr (!DO_OUTPOST) {
        return 0;
    }
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

int HandCraftedEvaluator<>::evaluateBishopPair(const Position& pos, Color color, int gpf) const {
    if constexpr (!DO_BISHOP_PAIR) {
        return 0;
    }
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

int HandCraftedEvaluator<>::evaluateAttacks(const Position& pos, Color c, int gpf) const {
    if constexpr (!DO_MOBILITY && !DO_NEAR_KING_ATK) {
        return 0;
    }

    int total = 0;

    Bitboard pawns = pos.getBitboard(Piece(c, PT_PAWN)) |
            pos.getBitboard(Piece(getOppositeColor(c), PT_PAWN));

    Bitboard occ = pawns;// | pos.getBitboard(Piece(c, PT_NONE));

    Bitboard nearKingSquares = getNearKingSquares(pos.getKingSquare(getOppositeColor(c)));

    for (PieceType pt = PT_KNIGHT; pt <= PT_QUEEN; ++pt) {
        Piece p = Piece(c, pt);
        Bitboard bb = pos.getBitboard(p);

        int nearKingAtkUnitScore = DO_NEAR_KING_ATK ? adjustScores(m_MgScores.nearKingAttacksScore[pt],
                                                m_EgScores.nearKingAttacksScore[pt],
                                                gpf) : 0;

        for (Square s: bb) {
            Bitboard atks = bbs::getPieceAttacks(s, occ, p);
            if constexpr (DO_MOBILITY) {
                Bitboard mobilityBB = atks & ~pawns;

                // Add mobility score
                int idx = std::min(mobilityBB.count(), ScoreTable::N_MOBILITY_SCORES - 1);
                total += adjustScores(m_MgScores.mobilityScores[pt][idx], m_EgScores.mobilityScores[pt][idx], gpf);
            }

            // Add near king attacks score
            if constexpr (DO_NEAR_KING_ATK) {
                Bitboard nearKingAtks = atks & nearKingSquares;
                total += nearKingAtks.count() * nearKingAtkUnitScore;
            }
        }
    }

    return total;
}

int HandCraftedEvaluator<>::evaluateHotmaps(const Position& pos, Color c, int gpf) const {
    if constexpr (!DO_PSQT && !DO_PAWN_CHAINS &&
                  !DO_PASSERS && !DO_CONN_PASSERS) {
        return 0;
    }
    int total = 0;

    Square ourKingSquare = pos.getKingSquare(c);

    // Evaluate pawn placement
    const Hotmap& mgHotmap = m_MgScores.pawnsHotmap;
    const Hotmap& egHotmap = m_MgScores.pawnsHotmap;
    Bitboard pawns = pos.getBitboard(Piece(c, PT_PAWN));
    Bitboard passedPawns = getPassedPawns(pos, c);
    Bitboard chainedPawns = getChainPawns(pos, c);
    Bitboard connectedPassers = passedPawns & chainedPawns;

    for (auto s: pawns) {
        if constexpr (DO_PSQT) {
            total += adjustScores(mgHotmap.valueAt(s, c),
                                  egHotmap.valueAt(s, c),
                                  gpf);
        }

        if constexpr (DO_PAWN_CHAINS) {
            // Perform pawn chain evaluation
            if (chainedPawns.contains(s)) {
                total += adjustScores(m_MgScores.chainedPawnsHotmap.valueAt(s, c),
                                      m_EgScores.chainedPawnsHotmap.valueAt(s, c),
                                      gpf);
            }
        }
        if (passedPawns.contains(s)) {
            if constexpr (DO_PASSERS) {
                total += adjustScores(m_MgScores.passersHotmap.valueAt(s, c),
                                      m_EgScores.passersHotmap.valueAt(s, c),
                                      gpf);
            }

            if constexpr (DO_CONN_PASSERS) {
                if (connectedPassers.contains(s)) {
                    total += adjustScores(m_MgScores.connectedPassersHotmap.valueAt(s, c),
                                          m_EgScores.connectedPassersHotmap.valueAt(s, c),
                                          gpf);
                }
            }
        }
    }

    // Evaluate king placement
    if constexpr (DO_PSQT) {
        total += adjustScores(m_MgScores.kingHotmap.valueAt(ourKingSquare, c),
                              m_EgScores.kingHotmap.valueAt(ourKingSquare, c),
                              gpf);
    }

    return total;
}

Bitboard HandCraftedEvaluator<>::getPassedPawns(const Position& pos, Color c) {
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

Bitboard HandCraftedEvaluator<>::getChainPawns(const Position& pos, Color c) {
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

int HandCraftedEvaluator<>::evaluateKingExposure(const Position& pos, Color c, int gpf) const {
    if constexpr (!DO_KING_EXPOSURE) {
        return 0;
    }

    int total = 0;
    Color them = getOppositeColor(c);
    Square theirKingSquare = pos.getKingSquare(them);
    Bitboard theirPawns = pos.getBitboard(Piece(them, PT_PAWN));

    Bitboard bishops = pos.getBitboard(Piece(c, PT_BISHOP));
    Bitboard rooks = pos.getBitboard(Piece(c, PT_ROOK));
    Bitboard queens = pos.getBitboard(Piece(c, PT_QUEEN));

    if (bbs::LIGHT_SQUARES.contains(theirKingSquare)) {
        bishops &= bbs::LIGHT_SQUARES;
    }
    else {
        bishops &= bbs::DARK_SQUARES;
    }

    Bitboard occ = pos.getBitboard(Piece(them, PT_PAWN));

    if (c == pos.getColorToMove()) {
        std::cout << "FEN: " << pos.toFen() << std::endl;
        std::cout << "unfiltered bishops:\n" << pos.getBitboard(Piece(c, PT_BISHOP));
        std::cout << "filtered bishops:\n" << bishops;
        std::cout << "popcount: " << bits::checkedPopcount(bbs::getBishopAttacks(theirKingSquare, occ) & ~theirPawns)
                  << std::endl;
        std::cout << std::endl;
    }

    // Compute bishop exposure
    if (bishops != 0 &&
        bits::checkedPopcount(bbs::getBishopAttacks(theirKingSquare, occ) & ~theirPawns) > 2) {
        total += adjustScores(m_MgScores.kingExposureScores[PT_BISHOP],
                              m_EgScores.kingExposureScores[PT_BISHOP],
                              gpf);
    }

    if (rooks != 0 &&
        bits::checkedPopcount(bbs::getRookAttacks(theirKingSquare, occ) & ~theirPawns) > 8) {
        total += adjustScores(m_MgScores.kingExposureScores[PT_ROOK],
                              m_EgScores.kingExposureScores[PT_ROOK],
                              gpf);
    }

    if (queens != 0 &&
        bits::checkedPopcount(bbs::getQueenAttacks(theirKingSquare, occ) & ~theirPawns) > 11) {
        total += adjustScores(m_MgScores.kingExposureScores[PT_QUEEN],
                              m_EgScores.kingExposureScores[PT_QUEEN],
                              gpf);
    }

    return total;
}

int HandCraftedEvaluator<>::evaluateBlockingPawns(const Position &pos, Color c, int gpf) const {
    if constexpr (!DO_BLOCKING_PAWN) {
        return 0;
    }
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

int HandCraftedEvaluator<>::getDrawScore() const {
    return 0;
}

int HandCraftedEvaluator<>::evaluate() const {
    const Position& pos = getPosition();

    // First, check if we are facing a known endgame
    EndgameData eg = endgame::identify(pos);
    if (eg.type == EG_UNKNOWN) {
        // Not a known endgame
        Color us = pos.getColorToMove();
        Color them = getOppositeColor(us);
        int total = evaluateClassic(pos);

        if (!pos.colorHasSufficientMaterial(us)) {
            return std::max(getDrawScore(), total);
        }
        else if (!pos.colorHasSufficientMaterial(them)) {
            return std::min(getDrawScore(), total);
        }

        return total;
    }
    if (eg.lhs == pos.getColorToMove()) {
        // Evaluate the endgame on our perspective
        return evaluateEndgame(pos, eg);
    }
    // Evaluate the endgame on their perspective
    return -evaluateEndgame(pos, eg);
}

int HandCraftedEvaluator<>::evaluateClassic(const Position& pos) const {
    Color us = pos.getColorToMove();
    Color them = getOppositeColor(us);

    int gpf = getGamePhaseFactor(pos);

    int total = 0;

    //total += evaluateMaterial(pos, us, gpf) - evaluateMaterial(pos, them, gpf);
    //total += evaluateHotmaps(pos, us, gpf) - evaluateHotmaps(pos, them, gpf);
    //total += evaluateAttacks(pos, us, gpf) - evaluateAttacks(pos, them, gpf);
    total += evaluateKingExposure(pos, us, gpf) - evaluateKingExposure(pos, them, gpf);
    //total += evaluateBlockingPawns(pos, us, gpf) - evaluateBlockingPawns(pos, them, gpf);
    //total += evaluateBishopPair(pos, us, gpf) - evaluateBishopPair(pos, them, gpf);

    //if (!pos.colorHasSufficientMaterial(us)) {
    //    total = std::min(getDrawScore() - 1, total);
    //}
    //if (!pos.colorHasSufficientMaterial(them)) {
    //    total = std::max(getDrawScore() + 1, total);
    //}

    return total;
}

int HandCraftedEvaluator<>::evaluateEndgame(const Position& pos, EndgameData egData) const {
    switch (egData.type) {
        // Drawn endgames
        case EG_KR_KN:
        case EG_KR_KB:
            return getDrawScore();

        case EG_KP_K:
            return evaluateKPK(pos, egData.lhs);

        case EG_KBN_K:
            return evaluateKBNK(pos, egData.lhs);

        case EG_KQ_K:
        case EG_KR_K:
        case EG_KBB_K:
            return evaluateCornerMateEndgame(pos, egData);

        default:
            // Not implemented endgame, resort to default evaluation:
            return evaluateClassic(pos);
    }

}

int HandCraftedEvaluator<>::evaluateCornerMateEndgame(const Position& pos, EndgameData eg) const {
    int materialScore;
    switch (eg.type) {
        case EG_KQ_K:
            materialScore = m_EgScores.materialScore[PT_QUEEN];
            break;

        case EG_KR_K:
            materialScore = m_EgScores.materialScore[PT_ROOK];
            break;

        case EG_KBB_K:
            materialScore = m_EgScores.materialScore[PT_BISHOP] * 2;
            break;

        default:
            materialScore = evaluateMaterial(pos, eg.lhs, 0);
            break;
    }

    constexpr int LOSING_KING_PSQT[] {
        70, 60, 50, 40, 40, 50, 60, 70,
        60, 50, 30, 20, 20, 30, 50, 60,
        50, 30,  0,  0,  0,  0, 30, 50,
        40, 20,  0,  0,  0,  0, 20, 40,
        40, 20,  0,  0,  0,  0, 20, 40,
        50, 30,  0,  0,  0,  0, 30, 50,
        60, 50, 30, 20, 20, 30, 50, 60,
        70, 60, 50, 40, 40, 50, 60, 70,
    };

    Color winner = eg.lhs;
    Color loser = getOppositeColor(winner);

    Square losingKingSquare = pos.getKingSquare(loser);
    Square winningKingSquare = pos.getKingSquare(winner);

    int kingDistanceScore = -6 * getManhattanDistance(losingKingSquare, winningKingSquare);
    int losingKingCornerScore = LOSING_KING_PSQT[losingKingSquare];

    constexpr int BASE_BONUS = 500;

    return materialScore + kingDistanceScore + losingKingCornerScore + BASE_BONUS;
}

int HandCraftedEvaluator<>::evaluateKPK(const Position &pos, Color lhs) const {
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

int HandCraftedEvaluator<>::evaluateKBNK(const Position &pos, Color lhs) const {
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

HandCraftedEvaluator<>::HandCraftedEvaluator<>() {
    m_MgScores = ScoreTable::getDefaultMiddlegameTable();
    m_EgScores = ScoreTable::getDefaultEndgameTable();
}

}