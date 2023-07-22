#include "hce.h"

#include "../../staticanalysis.h"

namespace lunachess::ai {

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

    // Some pre-computed values
    Bitboard ourPassers = staticanalysis::getPassedPawns(pos, us);
    Bitboard theirPassers = staticanalysis::getPassedPawns(pos, them);

    total += getMaterialScore(gpf, us) - getMaterialScore(gpf, them);
    total += getMobilityScore(gpf, us) - getMobilityScore(gpf, them);
    total += getPlacementScore(gpf, us) - getPlacementScore(gpf, them);
    total += getKingAttackScore(gpf, us) - getKingAttackScore(gpf, them);
    total += getIsolatedPawnsScore(gpf, us) - getIsolatedPawnsScore(gpf, them);
    total += getKnightOutpostScore(gpf, us) - getKnightOutpostScore(gpf, them);
    total += getBlockingPawnsScore(gpf, us) - getBlockingPawnsScore(gpf, them);
    total += getPassedPawnsScore(gpf, us, ourPassers) - getPassedPawnsScore(gpf, them, theirPassers);
    total += getBackwardPawnsScore(gpf, us) - getBackwardPawnsScore(gpf, them);
    total += getBishopPairScore(gpf, us) - getBishopPairScore(gpf, them);
    total += getKingPawnDistanceScore(gpf, us) - getKingPawnDistanceScore(gpf, them);
    total += getRooksScore(gpf, us, ourPassers) - getRooksScore(gpf, them, theirPassers);

    return total;
}

int HandCraftedEvaluator::getMaterialScore(int gpf, Color c) const {
    const auto& pos = getPosition();
    int total = 0;

    for (PieceType pt: { PT_PAWN, PT_KNIGHT, PT_BISHOP, PT_ROOK, PT_QUEEN } ) {
        total += pos.getBitboard(Piece(c, pt)).count() * m_Weights->material[pt].get(gpf);
    }

    return total;
}

int HandCraftedEvaluator::getPlacementScore(int gpf, Color c) const {
    const auto& pos = getPosition();
    int total = 0;

    // Kings
    Bitboard kingBB = pos.getBitboard(Piece(c, PT_KING));

    const PieceSquareTable& kingMgPST = m_Weights->kingPstMg;
    const PieceSquareTable& kingEgPST = m_Weights->kingPstEg;

    for (auto s: kingBB) {
        HCEWeight weight(kingMgPST.valueAt(s, c), kingEgPST.valueAt(s, c));
        total += weight.get(gpf);
    }

    // Queens
    Bitboard queenBB = pos.getBitboard(Piece(c, PT_KING));

    const PieceSquareTable& queenMgPST = m_Weights->queenPstMg;
    const PieceSquareTable& queenEgPST = m_Weights->queenPstEg;

    for (auto s: queenBB) {
        HCEWeight weight(queenMgPST.valueAt(s, c), queenEgPST.valueAt(s, c));
        total += weight.get(gpf);
    }

    // Pawns
    Bitboard pawnBB = pos.getBitboard(Piece(c, PT_PAWN));

    KingsDistribution kingsDistribution = staticanalysis::getKingsDistribution(pos, c);
    const PieceSquareTable& pawnMgPST = m_Weights->pawnPstsMg[kingsDistribution];
    const PieceSquareTable& pawnEgPST = m_Weights->pawnPstEg;

    for (auto s: pawnBB) {
        HCEWeight weight(pawnMgPST.valueAt(s, c), pawnEgPST.valueAt(s, c));

        total += weight.get(gpf);
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
//    auto occ = pos.getCompositeBitboard();
    auto occ = pos.getCompositeBitboard() & ~ourValuablePieces;

    // 'Target' squares are the squares that would "make sense" to jump to with our pieces.
    // This means squares that either have opposing pieces that we could capture or
    // are not being defended by opponent pawns.
    auto targetSquares = ~(theirPawnAttacks & ~theirValuablePieces);

    // Evaluate bishops
    auto ourBishops = pos.getBitboard(Piece(us, PT_BISHOP));
    for (auto s: ourBishops) {
        auto validSquares = bbs::getBishopAttacks(s, occ) & targetSquares;

        int scoreIdx = std::min(bits::popcount(validSquares), m_Weights->bishopMobilityScore.size() - 1);
        total += m_Weights->bishopMobilityScore[scoreIdx].get(gpf);
    }

    // Evaluate knights
    auto ourKnights = pos.getBitboard(Piece(us, PT_KNIGHT));
    for (auto s: ourKnights) {
        auto validSquares = bbs::getKnightAttacks(s) & targetSquares;

        int scoreIdx = std::min(bits::popcount(validSquares), m_Weights->knightMobilityScore.size() - 1);
        total += m_Weights->knightMobilityScore[scoreIdx].get(gpf);
    }

    // Evaluate rooks
    auto ourRooks = pos.getBitboard(Piece(us, PT_ROOK));
    for (auto s: ourRooks) {
        auto validSquares = bbs::getRookAttacks(s, occ) & targetSquares;
        auto validHorizontalSquares = validSquares & bbs::getRankBitboard(getRank(s));
        auto validVerticalSquares = validSquares & bbs::getFileBitboard(getFile(s));

        int horizontalScoreIdx = std::min(bits::popcount(validHorizontalSquares), m_Weights->rookHorizontalMobilityScore.size() - 1);
        total += m_Weights->rookHorizontalMobilityScore[horizontalScoreIdx].get(gpf);

        int verticalScoreIdx = std::min(bits::popcount(validVerticalSquares), m_Weights->rookVerticalMobilityScore.size() - 1);
        total += m_Weights->rookVerticalMobilityScore[verticalScoreIdx].get(gpf);
    }

    return total;
}

int HandCraftedEvaluator::getKnightOutpostScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard theirHalf = bbs::getBoardHalf(getOppositeColor(c));
    Bitboard knightOutposts = staticanalysis::getPieceOutposts(pos, Piece(c, PT_KNIGHT)) & theirHalf;

    return knightOutposts.count() * m_Weights->knightOutpostScore.get(gpf);
}

int HandCraftedEvaluator::getBlockingPawnsScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard blockingPawns = staticanalysis::getBlockingPawns(pos, c);

    return blockingPawns.count() * m_Weights->blockingPawnsScore.get(gpf);
}

int HandCraftedEvaluator::getIsolatedPawnsScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard connectedPawns = staticanalysis::getConnectedPawns(pos, c);
    Bitboard allPawns = pos.getBitboard(Piece(c, PT_PAWN));

    Bitboard isolatedPawns = allPawns & ~connectedPawns;

    return isolatedPawns.count() * m_Weights->isolatedPawnScore.get(gpf);
}

int HandCraftedEvaluator::getPassedPawnsScore(int gpf, Color c, Bitboard passedPawns) const {
    int total = 0;
    for (Square s: passedPawns) {
        int steps = stepsFromPromotion(s, c);
        int idx = std::min(size_t(steps), m_Weights->passedPawnScore.size()) - 1;
        total += m_Weights
                    ->passedPawnScore[idx]
                    .get(gpf);
    }

    return total;
}

int HandCraftedEvaluator::getBackwardPawnsScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard backwardPawns = staticanalysis::getBackwardPawns(pos, c);

    return backwardPawns.count() * m_Weights->backwardPawnScore.get(gpf);
}

int HandCraftedEvaluator::getKingPawnDistanceScore(int gpf, Color c) const {
    const auto& pos = getPosition();
    int total = 0;

    Bitboard ourKingSquare = pos.getKingSquare(c);
    Bitboard pawns = pos.getBitboard(Piece(CL_WHITE, PT_PAWN)) | pos.getBitboard(Piece(CL_BLACK, PT_PAWN));
    int individualScore = m_Weights->kingPawnDistanceScore.get(gpf);

    for (auto s: pawns) {
        auto distance = getChebyshevDistance(s, ourKingSquare);
        total += distance * individualScore;
    }

    return total;
}

int HandCraftedEvaluator::getBishopPairScore(int gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard ourBishops = pos.getBitboard(Piece(c, PT_BISHOP));
    Bitboard lsBishops = ourBishops & bbs::LIGHT_SQUARES;
    Bitboard dsBishops = ourBishops & bbs::DARK_SQUARES;

    return m_Weights->bishopPairScore.get(gpf) * std::min(lsBishops.count(), dsBishops.count());
}

int HandCraftedEvaluator::getRooksScore(int gpf, Color c, Bitboard passers) const {
    const auto& pos = getPosition();
    int total = 0;

    Bitboard occ = pos.getCompositeBitboard();
    Bitboard ourRooks = pos.getBitboard(Piece(c, PT_ROOK));
    if (ourRooks == 0) {
        return 0;
    }

    int openFileScore = m_Weights->rookOnOpenFile.get(gpf);
    int behindPasserScore = m_Weights->rookBehindPasser.get(gpf);

    for (Square s: ourRooks) {
        BoardFile file = getFile(s);
        FileState fileState = staticanalysis::getFileState(pos, file);

        if (fileState == FS_OPEN) {
            total += openFileScore;
        }

        Bitboard fileBB = bbs::getFileBitboard(file);
        Bitboard rookFileAtks = bbs::getRookAttacks(s, occ) & fileBB;

        if ((rookFileAtks & passers) != 0) {
            total += behindPasserScore;
        }
    }

    return total;
}

int HandCraftedEvaluator::getKingAttackScore(int gpf, Color us) const {
    const auto& pos = getPosition();
    Color them = getOppositeColor(us);

    int totalAttackPower = 0;
    Square theirKingSquare = pos.getKingSquare(getOppositeColor(us));
    Bitboard occ = pos.getCompositeBitboard();

    // Add attack power if our pieces can check the opponent's king
    constexpr int PIECE_CHK_POWER[] {
            0, 2, 6, 6, 6, 8, 10
    };

    for (PieceType pt: { PT_PAWN, PT_KNIGHT, PT_BISHOP, PT_ROOK, PT_QUEEN }) {
        Bitboard theirDefendedSquares = staticanalysis::getDefendedSquares(pos, them, pt);
        Piece p(us, pt);
        Bitboard pieceBB = pos.getBitboard(p);
        Bitboard atksFromKing = bbs::getPieceAttacks(theirKingSquare, occ, p);

        for (Square s: pieceBB) {
            Bitboard atks = bbs::getPieceAttacks(s, occ, p) & (~theirDefendedSquares);

            // Compute checks
            if ((atksFromKing & atks) != 0 && p.getType() != PT_KING) {
                totalAttackPower += PIECE_CHK_POWER[p.getType()];
            }
        }
    }

    // Add attack power if our queen can "touch" the opponent's
    // king without being captured
    constexpr int QUEENS_TOUCH_POWER = 15;
    Bitboard theirKingAtks = bbs::getKingAttacks(theirKingSquare);
    Bitboard ourQueensAttacks = pos.getAttacks(us, PT_QUEEN);
    Bitboard ourOpponentAttacks = staticanalysis::getDefendedSquares(pos, them, PT_QUEEN);
    Bitboard queenTouchSquares = theirKingAtks & ourQueensAttacks & (~ourOpponentAttacks) &
            staticanalysis::getDefendedSquares(pos, us, PT_ROOK);

    if (queenTouchSquares != 0) {
        totalAttackPower += QUEENS_TOUCH_POWER;
    }

    return m_Weights->kingAttackScore[std::min(size_t(totalAttackPower), m_Weights->kingAttackScore.size() - 1)];
}

int HandCraftedEvaluator::evaluateEndgame(const Position& pos, EndgameData egData) const {
    switch (egData.type) {
        // Drawn endgames
        case EG_KR_KN:
        case EG_KR_KB:
        case EG_KR_KR:
        case EG_KQ_KQ:
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
    int queenValue = m_Weights->material[PT_QUEEN].get(0);

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

    int base = m_Weights->material[PT_BISHOP].get(0) +
            m_Weights->material[PT_KNIGHT].get(0) +
            m_Weights->material[PT_PAWN].get(0) / 2;

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