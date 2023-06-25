#include "classicevaluator.h"

#include "../../staticanalysis.h"

namespace lunachess::ai {

PieceSquareTable g_DEFAULT_PAWN_PST_MG = {
    0,    0,    0,    0,    0,    0,    0,    0,
    300,  300,  300,  500,  500,  300,  300,  300,
    300,   300,  300,  500,  500,  300,  300,   300,
    0,    0,    0,  500,  500,    0,    0,    0,
    50,   50,  200,  500,  500,  20,   0,   0,
    0,   25,  80,  100,  100,  0,   0,   0,
    0,    0,    -150,    -200,    -200,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
};

PieceSquareTable g_DEFAULT_PAWN_PST_EG = {
    0,    0,    0,    0,    0,    0,    0,    0,
    700,  700,  700,  700,  700,  700,  700,  700,
    400,   400,  400,  400,  400,  400,  400,   400,
    350,    350,    350,  350,  350,    350,    350,    350,
    100,   100,  100,  100,  100,  100,   100,   100,
    0,   0,  0,  0,  0,  0,   0,   0,
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
};

PieceSquareTable g_DEFAULT_KING_PST_MG = {
    0,     0,    0,    0,     0,    0,    0,     0,
    -300, -300, -300, -300, -300, -300, -300, -300,
    -500, -500, -500, -500, -500, -500, -500, -500,
    -700, -700, -700, -700, -700, -700, -700, -700,
    -700, -700, -700, -700, -700, -700, -700, -700,
    -500, -500, -500, -500, -500, -500, -500, -500,
    -300, -300, -300, -300, -300, -300, -300, -300,
    100,   100,   50,  -300, -300,   -100,   350,   350,
};

PieceSquareTable g_DEFAULT_KING_PST_EG = {
    0,  100,  200,  300,  300,  200,   100,     0,
    100,  200,  300,  400,  400,  300,   200,   100,
    200,  300,  400,  500,  500,  400,   300,   200,
    300,  400,  500,  600,  600,  500,   400,   300,
    300,  400,  500,  600,  600,  500,   400,   300,
    200,  300,  400,  500,  500,  400,   300,   200,
    100,  200,  300,  400,  400,  300,   200,   100,
    0,  100,  200,  300,  300,  200,   100,     0,
};

int HandCraftedEvaluator::getGamePhaseFactor() const {
    auto& pos = getPosition();
    constexpr int KNIGHT_VAL = GPF_PIECE_VALUE_TABLE[PT_KNIGHT];
    constexpr int BISHOP_VAL = GPF_PIECE_VALUE_TABLE[PT_BISHOP];
    constexpr int ROOK_VAL   = GPF_PIECE_VALUE_TABLE[PT_ROOK];
    constexpr int QUEEN_VAL  = GPF_PIECE_VALUE_TABLE[PT_QUEEN];

    int nKnights = bits::popcount(pos.getBitboard(WHITE_KNIGHT) | pos.getBitboard(BLACK_KNIGHT));
    int nBishops = bits::popcount(pos.getBitboard(WHITE_BISHOP) | pos.getBitboard(BLACK_BISHOP));
    int nRooks   = bits::popcount(pos.getBitboard(WHITE_ROOK) | pos.getBitboard(BLACK_ROOK));
    int nQueens  = bits::popcount(pos.getBitboard(WHITE_QUEEN) | pos.getBitboard(BLACK_QUEEN));

    int total = nKnights * KNIGHT_VAL +
                nBishops * BISHOP_VAL +
                nRooks * ROOK_VAL +
                nQueens * QUEEN_VAL;

    int ret = total;
    return ret;
}

int HandCraftedEvaluator::evaluate() const {
    const auto& pos = getPosition();

    if ((m_ParamMask & BIT(HCEP_ENDGAME_THEORY)) == 0) {
        // If not asked, just evaluate the position normally
        return evaluateClassic(pos);
    }

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

int HandCraftedEvaluator::evaluateClassic(const Position& pos) const {
    int total = 0;
    Color us = pos.getColorToMove();
    Color them = getOppositeColor(us);

    int gpf = getGamePhaseFactor();

    if (m_ParamMask & BIT(HCEP_MATERIAL)) {
        total += getMaterialScore(gpf, us) - getMaterialScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_MOBILITY)) {
        total += getMobilityScore(gpf, us) - getMobilityScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_PLACEMENT)) {
        total += getPlacementScore(gpf, us) - getPlacementScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_KNIGHT_OUTPOSTS)) {
        total += getKnightOutpostScore(gpf, us) - getKnightOutpostScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_BLOCKING_PAWNS)) {
        total += getBlockingPawnsScore(gpf, us) - getBlockingPawnsScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_ISOLATED_PAWNS)) {
        total += getIsolatedPawnsScore(gpf, us) - getIsolatedPawnsScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_PASSED_PAWNS)) {
        total += getPassedPawnsScore(gpf, us) - getPassedPawnsScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_BACKWARD_PAWNS)) {
        total += getBackwardPawnsScore(gpf, us) - getBackwardPawnsScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_KING_EXPOSURE)) {
        total += getKingExposureScore(gpf, us) - getKingExposureScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_KING_PAWN_DISTANCE)) {
        total += getKingPawnDistanceScore(gpf, us) - getKingPawnDistanceScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_PAWN_SHIELD)) {
        total += getPawnShieldScore(gpf, us) - getPawnShieldScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_TROPISM)) {
        total += getTropismScore(gpf, us) - getTropismScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_BISHOP_PAIR)) {
        total += getBishopPairScore(gpf, us) - getBishopPairScore(gpf, them);
    }
    if (m_ParamMask & BIT(HCEP_KING_ATTACK)) {
        total += getKingAttackScore(gpf, us) - getKingAttackScore(gpf, them);
    }

    return total;
}

int HandCraftedEvaluator::getMaterialScore(int gpf, Color c) const {
    const auto& pos = getPosition();
    int total = 0;

    for (PieceType pt: { PT_PAWN, PT_KNIGHT, PT_BISHOP, PT_ROOK, PT_QUEEN } ) {
        total += pos.getBitboard(Piece(c, pt)).count() * m_Weights.material[pt].get(gpf);
    }

    return total;
}

int HandCraftedEvaluator::getPlacementScore(int gpf, Color c) const {
    const auto& pos = getPosition();
    int total = 0;

    for (auto pt: { PT_PAWN, PT_KING }) {
        auto bb = pos.getBitboard(Piece(c, pt));

        const auto& mgPST = m_Weights.mgPSTs[pt];
        const auto& egPST = m_Weights.egPSTs[pt];

        for (auto s: bb) {
            HCEWeight weight(mgPST.valueAt(s, c), egPST.valueAt(s, c));
            total += weight.get(gpf);
        }
    }

    return total;
}

int HandCraftedEvaluator::getMobilityScore(int gpf, Color us) const {
    const auto& pos = getPosition();
    int total = 0;

    Color them = getOppositeColor(us);

    auto theirPawnAttacks = pos.getAttacks(them, PT_PAWN);
    auto theirValuablePieces = pos.getBitboard(Piece(them, PT_NONE)) & ~pos.getBitboard(Piece(them, PT_PAWN));
    auto ourValuablePieces = pos.getBitboard(Piece(us, PT_NONE)) & ~pos.getBitboard(Piece(us, PT_PAWN));
    auto occ = pos.getCompositeBitboard() & ~ourValuablePieces;

    // 'Target' squares are the squares that would "make sense" to jump to with our pieces.
    // This means squares that either have opposing pieces that we could capture or
    // are not being defended by opponent pawns.
    auto targetSquares = ~(theirPawnAttacks & ~theirValuablePieces);

    // Evaluate bishops
    auto ourBishops = pos.getBitboard(Piece(us, PT_BISHOP));
    for (auto s: ourBishops) {
        auto validSquares = bbs::getBishopAttacks(s, occ) & targetSquares;

        int scoreIdx = std::min(bits::popcount(validSquares), m_Weights.bishopMobilityScore.size() - 1);
        total += m_Weights.bishopMobilityScore[scoreIdx].get(gpf);
    }

    // Evaluate knights
    auto ourKnights = pos.getBitboard(Piece(us, PT_KNIGHT));
    for (auto s: ourKnights) {
        auto validSquares = bbs::getKnightAttacks(s) & targetSquares;

        int scoreIdx = std::min(bits::popcount(validSquares), m_Weights.knightMobilityScore.size() - 1);
        total += m_Weights.knightMobilityScore[scoreIdx].get(gpf);
    }

    // Evaluate rooks
    auto ourRooks = pos.getBitboard(Piece(us, PT_ROOK));
    for (auto s: ourRooks) {
        auto validSquares = bbs::getRookAttacks(s, occ) & targetSquares;
        auto validHorizontalSquares = validSquares & bbs::getRankBitboard(getRank(s));
        auto validVerticalSquares = validSquares & bbs::getFileBitboard(getFile(s));

        int horizontalScoreIdx = std::min(bits::popcount(validHorizontalSquares), m_Weights.rookHorizontalMobilityScore.size() - 1);
        total += m_Weights.rookHorizontalMobilityScore[horizontalScoreIdx].get(gpf);

        int verticalScoreIdx = std::min(bits::popcount(validVerticalSquares), m_Weights.rookVerticalMobilityScore.size() - 1);
        total += m_Weights.rookVerticalMobilityScore[verticalScoreIdx].get(gpf);
    }

    return total / 2;
}

int HandCraftedEvaluator::getKnightOutpostScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard theirHalf = bbs::getBoardHalf(getOppositeColor(c));
    Bitboard knightOutposts = staticanalysis::getPieceOutposts(pos, Piece(c, PT_KNIGHT)) & theirHalf;

    return knightOutposts.count() * m_Weights.knightOutpostScore.get(gpf);
}

int HandCraftedEvaluator::getBlockingPawnsScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard blockingPawns = staticanalysis::getBlockingPawns(pos, c);

    return blockingPawns.count() * m_Weights.blockingPawnsScore.get(gpf);
}

int HandCraftedEvaluator::getIsolatedPawnsScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard connectedPawns = staticanalysis::getConnectedPawns(pos, c);
    Bitboard allPawns = pos.getBitboard(Piece(c, PT_PAWN));

    Bitboard isolatedPawns = allPawns & ~connectedPawns;

    return isolatedPawns.count() * m_Weights.isolatedPawnScore.get(gpf);
}

int HandCraftedEvaluator::getPassedPawnsScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard passedPawns = staticanalysis::getPassedPawns(pos, c);

    return passedPawns.count() * m_Weights.passedPawnScore.get(gpf);
}

int HandCraftedEvaluator::getBackwardPawnsScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard backwardPawns = staticanalysis::getBackwardPawns(pos, c);

    return backwardPawns.count() * m_Weights.backwardPawnScore.get(gpf);
}

int HandCraftedEvaluator::getKingExposureScore(int gpf, Color us) const {
    const auto& pos = getPosition();
    int total = 0;

    Color them = getOppositeColor(us);
    Bitboard occ = pos.getCompositeBitboard();
    Bitboard ourPawnAttacks = pos.getAttacks(us, PT_PAWN);
    Bitboard ourKingSquare = pos.getKingSquare(us);

    Bitboard theirKnightAttacks = (pos.getAttacks(them, PT_KNIGHT) & ~ourPawnAttacks);
    Bitboard theirBishopAttacks = (pos.getAttacks(them, PT_BISHOP) & ~ourPawnAttacks);
    Bitboard theirRookAttacks = (pos.getAttacks(them, PT_ROOK) & ~ourPawnAttacks);
    Bitboard theirQueenAttacks = (pos.getAttacks(them, PT_QUEEN) & ~ourPawnAttacks);

    Bitboard theirKnightDominion = pos.getBitboard(Piece(them, PT_KNIGHT)) | theirKnightAttacks;
    Bitboard theirBishopDominion = pos.getBitboard(Piece(them, PT_BISHOP)) | theirBishopAttacks;
    Bitboard theirRookDominion = pos.getBitboard(Piece(them, PT_ROOK))   | theirRookAttacks;
    Bitboard theirQueenDominion = pos.getBitboard(Piece(them, PT_QUEEN))  | theirQueenAttacks;

    Bitboard knightAtksFromKing = bbs::getKnightAttacks(ourKingSquare);
    Bitboard bishopAtksFromKing = bbs::getBishopAttacks(ourKingSquare, occ);
    Bitboard rookAtksFromKing = bbs::getRookAttacks(ourKingSquare, occ);
    Bitboard queenAtksFromKing = bbs::getQueenAttacks(ourKingSquare, occ);

    if ((theirKnightDominion & knightAtksFromKing) != 0) {
        total += m_Weights.knightExposureScore.get(gpf);
    }
    if ((theirBishopDominion & bishopAtksFromKing) != 0) {
        total += m_Weights.bishopExposureScore.get(gpf);
    }
    if ((theirRookDominion & rookAtksFromKing) != 0) {
        total += m_Weights.rookExposureScore.get(gpf);
    }
    if ((theirQueenDominion & queenAtksFromKing) != 0) {
        total += m_Weights.queenExposureScore.get(gpf);
    }

    return total;
}

int HandCraftedEvaluator::getKingPawnDistanceScore(int gpf, Color c) const {
    const auto& pos = getPosition();
    int total = 0;

    Bitboard ourKingSquare = pos.getKingSquare(c);
    Bitboard pawns = pos.getBitboard(Piece(CL_WHITE, PT_PAWN)) | pos.getBitboard(Piece(CL_BLACK, PT_PAWN));
    int individualScore = m_Weights.kingPawnDistanceScore.get(gpf);

    for (auto s: pawns) {
        auto distance = getChebyshevDistance(s, ourKingSquare);
        total += distance * individualScore;
    }

    return total;
}

int HandCraftedEvaluator::getPawnShieldScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard ourKingSquare = pos.getKingSquare(c);
    Bitboard ourPawns = pos.getBitboard(Piece(c, PT_PAWN));
    Bitboard pawnShieldBB = bbs::getPawnShieldBitboard(ourKingSquare, c);
    Bitboard ourPawnShield = pawnShieldBB & ourPawns;

    int scoreIdx = std::min(bits::popcount(ourPawnShield), m_Weights.pawnShieldScore.size() - 1);
    return m_Weights.pawnShieldScore[scoreIdx].get(gpf);
}

int HandCraftedEvaluator::getTropismScore(int gpf, Color us) const {
    const auto& pos = getPosition();
    int total = 0;

    Color them = getOppositeColor(us);
    Square theirKing = pos.getKingSquare(them);

    Bitboard ourKnights = pos.getBitboard(Piece(us, PT_KNIGHT));
    Bitboard ourBishops = pos.getBitboard(Piece(us, PT_BISHOP));
    Bitboard ourRooks = pos.getBitboard(Piece(us, PT_ROOK));
    Bitboard ourQueens = pos.getBitboard(Piece(us, PT_QUEEN));

    for (auto s: ourKnights) {
        int distance = getChebyshevDistance(s, theirKing);
        int scoreIdx = std::min(distance, (int)m_Weights.knightTropismScore.size() - 1);
        total += m_Weights.knightTropismScore[scoreIdx].get(gpf);
    }

    for (auto s: ourBishops) {
        int distance = getChebyshevDistance(s, theirKing);
        int scoreIdx = std::min(distance, (int)m_Weights.bishopTropismScore.size() - 1);
        total += m_Weights.bishopTropismScore[scoreIdx].get(gpf);
    }

    for (auto s: ourRooks) {
        int distance = getManhattanDistance(s, theirKing);
        int scoreIdx = std::min(distance, (int)m_Weights.rookTropismScore.size() - 1);
        total += m_Weights.rookTropismScore[scoreIdx].get(gpf);
    }

    for (auto s: ourQueens) {
        int distance = getChebyshevDistance(s, theirKing);
        int scoreIdx = std::min(distance, (int)m_Weights.queenTropismScore.size() - 1);
        total += m_Weights.queenTropismScore[scoreIdx].get(gpf);
    }

    return total;
}

int HandCraftedEvaluator::getBishopPairScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard ourBishops = pos.getBitboard(Piece(c, PT_BISHOP));
    Bitboard lsBishops = ourBishops & bbs::LIGHT_SQUARES;
    Bitboard dsBishops = ourBishops & bbs::DARK_SQUARES;

    return m_Weights.bishopPairScore.get(gpf) * std::min(lsBishops.count(), dsBishops.count());
}

int HandCraftedEvaluator::getKingAttackScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    constexpr int PIECE_ATK_POWER[] {
        0, 1, 3, 3, 6, 12, 1
    };

    int totalAttackPower = 0;
    Square theirKingSquare = pos.getKingSquare(getOppositeColor(c));
    Bitboard nearKingSquares = bbs::getNearKingSquares(theirKingSquare);
    Bitboard ourPieces = pos.getBitboard(Piece(c, PT_NONE));
    Bitboard occ = pos.getCompositeBitboard();

    for (Square s: ourPieces) {
        Piece p = pos.getPieceAt(s);
        Bitboard atks = bbs::getPieceAttacks(s, occ, p);
        if ((atks & nearKingSquares) != 0) {
            totalAttackPower += PIECE_ATK_POWER[p.getType()];
        }
    }

    return m_Weights.kingAttackScore[std::min(size_t(totalAttackPower), m_Weights.kingAttackScore.size() - 1)];
}

int HandCraftedEvaluator::evaluateEndgame(const Position& pos, EndgameData egData) const {
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

int HandCraftedEvaluator::evaluateKPK(const Position &pos, Color lhs) const {
    int queenValue = m_Weights.material[PT_QUEEN].get(0);

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

int HandCraftedEvaluator::evaluateKBNK(const Position &pos, Color lhs) const {
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

    int base = m_Weights.material[PT_BISHOP].get(0) +
            m_Weights.material[PT_KNIGHT].get(0) +
            m_Weights.material[PT_PAWN].get(0) / 2;

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

}