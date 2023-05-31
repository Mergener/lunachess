#include "classicevaluator.h"

#include "../../staticanalysis.h"

namespace lunachess::ai {

PieceSquareTable g_DEFAULT_PAWN_PST_MG = {
    0,    0,    0,    0,    0,    0,    0,    0,
    100,  200,  300,  400,  400,  300,  200,  100,
    50,   100,  150,  250,  250,  150,  100,   50,
    0,    0,    0,  200,  200,    0,    0,    0,
    50,   50,  80,  150,  150,  20,   0,   0,
    0,   25,  80,  100,  100,  0,   0,   0,
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
};

PieceSquareTable g_DEFAULT_PAWN_PST_EG = {
    0,    0,    0,    0,    0,    0,    0,    0,
    250,  250,  250,  250,  250,  250,  250,  250,
    200,   200,  200,  200,  200,  200,  200,   200,
    150,    150,    150,  150,  150,    150,    150,    150,
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
    100,   100,   50,  -200, -200,   0,   100,   100,
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

    return total;
}

int HandCraftedEvaluator::getKnightOutpostScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard knightOutposts = staticanalysis::getPieceOutposts(pos, Piece(c, PT_KNIGHT));

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