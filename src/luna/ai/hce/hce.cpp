#include "hce.h"

#include "../../staticanalysis.h"

namespace lunachess::ai {

i32 HandCraftedEvaluator::getGamePhaseFactor() const {
    auto& pos = getPosition();

    constexpr i32 KNIGHT_VAL = GPF_PIECE_VALUE_TABLE[PT_KNIGHT];
    constexpr i32 BISHOP_VAL = GPF_PIECE_VALUE_TABLE[PT_BISHOP];
    constexpr i32 ROOK_VAL   = GPF_PIECE_VALUE_TABLE[PT_ROOK];
    constexpr i32 QUEEN_VAL  = GPF_PIECE_VALUE_TABLE[PT_QUEEN];

    i32 nKnights = bits::popcount(pos.getBitboard(WHITE_KNIGHT) | pos.getBitboard(BLACK_KNIGHT));
    i32 nBishops = bits::popcount(pos.getBitboard(WHITE_BISHOP) | pos.getBitboard(BLACK_BISHOP));
    i32 nRooks   = bits::popcount(pos.getBitboard(WHITE_ROOK)   | pos.getBitboard(BLACK_ROOK));
    i32 nQueens  = bits::popcount(pos.getBitboard(WHITE_QUEEN)  | pos.getBitboard(BLACK_QUEEN));

    i32 total = nKnights * KNIGHT_VAL +
                nBishops * BISHOP_VAL +
                nRooks   * ROOK_VAL   +
                nQueens  * QUEEN_VAL;

    i32 ret = total;
    return ret;
}

i32 HandCraftedEvaluator::evaluate() const {
    const auto& pos = getPosition();

    // First, check if we are facing a known endgame
    EndgameData eg = endgame::identify(pos);
    if (eg.type == EG_UNKNOWN) {
        // Not a known endgame
        return evaluateClassic(pos, pos.getColorToMove());
    }
    if (eg.lhs == pos.getColorToMove()) {
        // Evaluate the endgame on our perspective
        return evaluateEndgame(pos, eg);
    }
    // Evaluate the endgame on their perspective
    return -evaluateEndgame(pos, eg);
}

i32 HandCraftedEvaluator::evaluateClassic(const Position& pos, Color us) const {
    i32 gpf    = getGamePhaseFactor();
    i32 tempo  = m_Weights->tempoScore.get(gpf);
    i32 total  = us == pos.getColorToMove() ? tempo : -tempo;
    Color them = getOppositeColor(us);

    // Some pre-computed values
    Bitboard ourPassers   = staticanalysis::getPassedPawns(pos, us);
    Bitboard theirPassers = staticanalysis::getPassedPawns(pos, them);

    // Compute evaluation features
    total += getMaterialScore(gpf, us) - getMaterialScore(gpf, them);
    total += getMobilityScore(gpf, us) - getMobilityScore(gpf, them);
    total += getPlacementScore(gpf, us) - getPlacementScore(gpf, them);
    total += getKingAttackScore(gpf, us) - getKingAttackScore(gpf, them);
    total += getIsolatedPawnsScore(gpf, us) - getIsolatedPawnsScore(gpf, them);
    total += getKnightOutpostScore(gpf, us) - getKnightOutpostScore(gpf, them);
    total += getBlockingPawnsScore(gpf, us) - getBlockingPawnsScore(gpf, them);
    total += getBackwardPawnsScore(gpf, us) - getBackwardPawnsScore(gpf, them);
    total += getBishopPairScore(gpf, us) - getBishopPairScore(gpf, them);
    total += getKingPawnDistanceScore(gpf, us) - getKingPawnDistanceScore(gpf, them);
    total += getRooksScore(gpf, us, ourPassers) - getRooksScore(gpf, them, theirPassers);
    total += getPassedPawnsScore(gpf, us, ourPassers) - getPassedPawnsScore(gpf, them, theirPassers);

    return total;
}

i32 HandCraftedEvaluator::getMaterialScore(i32 gpf, Color c) const {
    const auto& pos = getPosition();
    i32 total = 0;

    for (PieceType pt: { PT_PAWN, PT_KNIGHT, PT_BISHOP, PT_ROOK, PT_QUEEN } ) {
        total += pos.getBitboard(Piece(c, pt)).count() * m_Weights->material[pt].get(gpf);
    }

    return total;
}

static i32 evaluatePST(Bitboard bb, Color c,
                       const PieceSquareTable& mg,
                       const PieceSquareTable& eg,
                       i32 gpf) {
    i32 total = 0;
    for (auto s: bb) {
        HCEWeight weight(mg.valueAt(s, c), eg.valueAt(s, c));
        total += weight.get(gpf);
    }
    return total;
}

i32 HandCraftedEvaluator::getPlacementScore(i32 gpf, Color c) const {
    const auto& pos = getPosition();
    i32 total = 0;

    // Kings
    Bitboard kingBB = pos.getBitboard(Piece(c, PT_KING));

    const PieceSquareTable& kingMgPST = m_Weights->kingPstMg;
    const PieceSquareTable& kingEgPST = m_Weights->kingPstEg;

    total += evaluatePST(kingBB, c, kingMgPST, kingEgPST, gpf);

    // Knights
    {
        Bitboard knightBB = pos.getBitboard(Piece(c, PT_KNIGHT));

        const PieceSquareTable& knightMgPST = m_Weights->knightPstMg;
        const PieceSquareTable& knightEgPST = m_Weights->knightPstEg;

        total += evaluatePST(knightBB, c, knightMgPST, knightEgPST, gpf);
    }

    // Knights
    {
        Bitboard bishopBB = pos.getBitboard(Piece(c, PT_BISHOP));

        const PieceSquareTable& bishopMgPST = m_Weights->bishopPstMg;
        const PieceSquareTable& bishopEgPST = m_Weights->bishopPstEg;

        total += evaluatePST(bishopBB, c, bishopMgPST, bishopEgPST, gpf);
    }

    // Rooks
    {
        Bitboard rookBB = pos.getBitboard(Piece(c, PT_ROOK));

        const PieceSquareTable& rookMgPST = m_Weights->rookPstMg;
        const PieceSquareTable& rookEgPST = m_Weights->rookPstEg;

        total += evaluatePST(rookBB, c, rookMgPST, rookEgPST, gpf);
    }

    // Queens
    {
        Bitboard queenBB = pos.getBitboard(Piece(c, PT_QUEEN));

        const PieceSquareTable& queenMgPST = m_Weights->queenPstMg;
        const PieceSquareTable& queenEgPST = m_Weights->queenPstEg;

        total += evaluatePST(queenBB, c, queenMgPST, queenEgPST, gpf);
    }

    // Pawns
    Bitboard pawnBB = pos.getBitboard(Piece(c, PT_PAWN));

    KingsDistribution kingsDistribution = staticanalysis::getKingsDistribution(pos, c);
    const PieceSquareTable& pawnMgPST = m_Weights->pawnPstsMg[kingsDistribution];
    const PieceSquareTable& pawnEgPST = m_Weights->pawnPstEg;

    total += evaluatePST(pawnBB, c, pawnMgPST, pawnEgPST, gpf);


    return total;
}

i32 HandCraftedEvaluator::getMobilityScore(i32 gpf, Color us) const {
    const auto& pos = getPosition();
    i32 total = 0;

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

        i32 scoreIdx = std::min(bits::popcount(validSquares), m_Weights->bishopMobilityScore.size() - 1);
        total += m_Weights->bishopMobilityScore[scoreIdx].get(gpf);
    }

    // Evaluate knights
    auto ourKnights = pos.getBitboard(Piece(us, PT_KNIGHT));
    for (auto s: ourKnights) {
        auto validSquares = bbs::getKnightAttacks(s) & targetSquares;

        i32 scoreIdx = std::min(bits::popcount(validSquares), m_Weights->knightMobilityScore.size() - 1);
        total += m_Weights->knightMobilityScore[scoreIdx].get(gpf);
    }

    // Evaluate rooks
    auto ourRooks = pos.getBitboard(Piece(us, PT_ROOK));
    for (auto s: ourRooks) {
        auto validSquares = bbs::getRookAttacks(s, occ) & targetSquares;
        auto validHorizontalSquares = validSquares & bbs::getRankBitboard(getRank(s));
        auto validVerticalSquares = validSquares & bbs::getFileBitboard(getFile(s));

        i32 horizontalScoreIdx = std::min(bits::popcount(validHorizontalSquares), m_Weights->rookHorizontalMobilityScore.size() - 1);
        total += m_Weights->rookHorizontalMobilityScore[horizontalScoreIdx].get(gpf);

        i32 verticalScoreIdx = std::min(bits::popcount(validVerticalSquares), m_Weights->rookVerticalMobilityScore.size() - 1);
        total += m_Weights->rookVerticalMobilityScore[verticalScoreIdx].get(gpf);
    }

    return total;
}

i32 HandCraftedEvaluator::getKnightOutpostScore(i32 gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard theirHalf = bbs::getBoardHalf(getOppositeColor(c));
    Bitboard knightOutposts = staticanalysis::getPieceOutposts(pos, Piece(c, PT_KNIGHT)) & theirHalf;

    return knightOutposts.count() * m_Weights->knightOutpostScore.get(gpf);
}

i32 HandCraftedEvaluator::getBlockingPawnsScore(i32 gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard blockingPawns = staticanalysis::getBlockingPawns(pos, c);

    return blockingPawns.count() * m_Weights->blockingPawnsScore.get(gpf);
}

i32 HandCraftedEvaluator::getIsolatedPawnsScore(i32 gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard connectedPawns = staticanalysis::getConnectedPawns(pos, c);
    Bitboard allPawns = pos.getBitboard(Piece(c, PT_PAWN));

    Bitboard isolatedPawns = allPawns & ~connectedPawns;

    return isolatedPawns.count() * m_Weights->isolatedPawnScore.get(gpf);
}

i32 HandCraftedEvaluator::getPassedPawnsScore(i32 gpf, Color c, Bitboard passedPawns) const {
    i32 total = 0;
    for (Square s: passedPawns) {
        i32 steps = stepsFromPromotion(s, c);
        i32 idx = std::min(size_t(steps), m_Weights->passedPawnScore.size()) - 1;
        total += m_Weights
                    ->passedPawnScore[idx]
                    .get(gpf);
    }

    return total;
}

i32 HandCraftedEvaluator::getBackwardPawnsScore(i32 gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard backwardPawns = staticanalysis::getBackwardPawns(pos, c);

    return backwardPawns.count() * m_Weights->backwardPawnScore.get(gpf);
}

i32 HandCraftedEvaluator::getKingPawnDistanceScore(i32 gpf, Color c) const {
    const auto& pos = getPosition();
    i32 total = 0;

    Bitboard ourKingSquare = pos.getKingSquare(c);
    Bitboard pawns = pos.getBitboard(Piece(CL_WHITE, PT_PAWN)) | pos.getBitboard(Piece(CL_BLACK, PT_PAWN));
    i32 individualScore = m_Weights->kingPawnDistanceScore.get(gpf);

    for (auto s: pawns) {
        auto distance = getChebyshevDistance(s, ourKingSquare);
        total += distance * individualScore;
    }

    return total;
}

i32 HandCraftedEvaluator::getBishopPairScore(i32 gpf, Color c) const {
    const auto& pos = getPosition();

    Bitboard ourBishops = pos.getBitboard(Piece(c, PT_BISHOP));
    Bitboard lsBishops = ourBishops & bbs::LIGHT_SQUARES;
    Bitboard dsBishops = ourBishops & bbs::DARK_SQUARES;

    return m_Weights->bishopPairScore.get(gpf) * std::min(lsBishops.count(), dsBishops.count());
}

i32 HandCraftedEvaluator::getRooksScore(i32 gpf, Color c, Bitboard passers) const {
    const auto& pos = getPosition();
    i32 total = 0;

    Bitboard occ      = pos.getCompositeBitboard();
    Bitboard ourRooks = pos.getBitboard(Piece(c, PT_ROOK));
    if (ourRooks == 0) {
        return 0;
    }
    Bitboard ourPawns = pos.getBitboard(Piece(c, PT_PAWN));

    i32 openFileScore     = m_Weights->rookOnOpenFile.get(gpf);
    i32 behindPasserScore = m_Weights->rookBehindPasser.get(gpf);

    for (Square s: ourRooks) {
        BoardFile file     = getFile(s);
        Bitboard fileBB    = bbs::getFileBitboard(file);
        Bitboard filePawns = ourPawns & fileBB;

        if (filePawns == 0) {
            total += openFileScore;
        }

        Bitboard rookFileAtks = bbs::getRookAttacks(s, occ) & fileBB;

        if ((rookFileAtks & passers) != 0) {
            total += behindPasserScore;
        }
    }

    return total;
}


i32 HandCraftedEvaluator::getCheckPower(i32 gpf, Color us) const {
    const auto& pos = getPosition();
    Color them = getOppositeColor(us);

    Square theirKingSquare = pos.getKingSquare(getOppositeColor(us));
    Bitboard occ = pos.getCompositeBitboard();
    i32 total    = 0;

    // Add attack power if our pieces can check the opponent's king
    for (PieceType pt: { PT_PAWN, PT_KNIGHT, PT_BISHOP, PT_ROOK, PT_QUEEN }) {
        Bitboard theirDefendedSquares = staticanalysis::getDefendedSquares(pos, them, pt);
        Piece p(us, pt);
        Bitboard pieceBB = pos.getBitboard(p);
        Bitboard atksFromKing = bbs::getPieceAttacks(theirKingSquare, occ, p);
        i32 checkPower = m_Weights->pieceCheckPower[pt].get(gpf);

        for (Square s: pieceBB) {
            Bitboard atks = bbs::getPieceAttacks(s, occ, p) & (~theirDefendedSquares);

            // Compute checks
            if ((atksFromKing & atks) != 0 && p.getType() != PT_KING) {
                total += checkPower;
            }
        }
    }

    return total;
}

i32 HandCraftedEvaluator::getQueenTouchPower(i32 gpf, Color us) const {
    const auto& pos = getPosition();
    Color them = getOppositeColor(us);
    Square theirKingSquare = pos.getKingSquare(getOppositeColor(us));

    // Add attack power if our queen can "touch" the opponent's
    // king without being captured
    Bitboard theirKingAtks      = bbs::getKingAttacks(theirKingSquare);
    Bitboard ourQueensAttacks   = pos.getAttacks(us, PT_QUEEN);
    Bitboard ourOpponentAttacks = staticanalysis::getDefendedSquares(pos, them, PT_QUEEN);
    Bitboard queenTouchSquares  = theirKingAtks & ourQueensAttacks & (~ourOpponentAttacks) &
                                  staticanalysis::getDefendedSquares(pos, us, PT_ROOK);

    if (queenTouchSquares != 0) {
        return m_Weights->queenTouchPower.get(gpf);
    }

    return 0;
}

i32 HandCraftedEvaluator::getKingAttackScore(i32 gpf, Color us) const {
    i32 totalAttackPower = 0;

    totalAttackPower += getQueenTouchPower(gpf, us);
    totalAttackPower += getCheckPower(gpf, us);

    size_t idx = std::min(size_t(totalAttackPower) >> 4, m_Weights->kingAttackScore.size() - 1);

    return m_Weights->kingAttackScore[idx];
}

i32 HandCraftedEvaluator::evaluateEndgame(const Position& pos, EndgameData egData) const {
    switch (egData.type) {
        // Drawn endgames
        case EG_KR_KN:
        case EG_KR_KB:
        case EG_KR_KR:
        case EG_KQ_KQ:
            return getDrawScore();

        case EG_KBP_K:
            return evaluateKBPK(pos, egData.lhs);

        case EG_KP_K:
            return evaluateKPK(pos, egData.lhs);

        case EG_KBN_K:
            return evaluateKBNK(pos, egData.lhs);

        default:
            // Not implemented endgame, resort to default evaluation:
            return evaluateClassic(pos, egData.lhs);
    }

}

i32 HandCraftedEvaluator::evaluateKPK(const Position &pos, Color lhs) const {
    i32 queenValue = m_Weights->material[PT_QUEEN].get(0);

    Color rhs = getOppositeColor(lhs);
    Square pawnSquare      = *pos.getBitboard(Piece(lhs, PT_PAWN)).begin();
    Square enemyKingSquare = pos.getKingSquare(rhs);

    if (!endgame::isInsideTheSquare(pawnSquare, enemyKingSquare, lhs, pos.getColorToMove())) {
        // King outside of square, pawn can promote freely.
        BoardRank promRank = getPromotionRank(lhs);
        BoardRank pawnRank = getRank(pawnSquare);
        i32 dist = std::abs(promRank - pawnRank);
        return queenValue - dist * 100;
    }

    return evaluateClassic(pos, lhs);
}

i32 HandCraftedEvaluator::evaluateKBPK(const Position& pos, Color lhs) const {
    constexpr Bitboard A_H_FILES   = bbs::getFileBitboard(FL_A) | bbs::getFileBitboard(FL_H);

    Bitboard pawnBB   = pos.getBitboard(Piece(lhs, PT_PAWN));
    Square pawnSquare = *pawnBB.begin();
    i32 winningScore  = (m_Weights->material[PT_BISHOP].eg + m_Weights->material[PT_PAWN].eg) * 2 +
            (5 - stepsFromPromotion(pawnSquare, lhs)) * 400;

    if (!A_H_FILES.contains(pawnBB)) {
        // Winning for the side with the bishop
        return winningScore;
    }

    // The pawn is on either the H or A file. Check if the promotion
    // square is the same color as the bishop.
    BoardFile pawnFile     = getFile(pawnSquare);
    Bitboard bishopBB      = pos.getBitboard(Piece(lhs, PT_BISHOP));
    Square promSquare      = getPromotionSquare(lhs, pawnFile);
    Bitboard bishopComplex = (bbs::LIGHT_SQUARES & bishopBB)
            ? bbs::LIGHT_SQUARES
            : bbs::DARK_SQUARES;

    if (bishopComplex.contains(promSquare)) {
        // Winning for the side with the bishop
        return winningScore;
    }

    // Pawn is not on the same color complex as the bishop.
    Color rhs          = getOppositeColor(lhs);
    Bitboard rhsKingBB = pos.getBitboard(Piece(rhs, PT_KING));
    Bitboard drawBB    = rhsKingBB | (bbs::getKingAttacks(promSquare));
    if (drawBB & rhsKingBB) {
        // King is on the area that prevents the win, endgame is a draw.
        return getDrawScore();
    }

    if (!endgame::isInsideTheSquare(pawnSquare, *rhsKingBB.begin(), lhs, pos.getColorToMove())) {
        // King is not on the square, the pawn can just march along the way.
        return winningScore;
    }

    // Uncovered scenarios, fallback to standard evaluation methods
    return getMaterialScore(0, lhs)  - getMaterialScore(0, rhs) +
           getPlacementScore(0, lhs) - getPlacementScore(0, rhs);
}

i32 HandCraftedEvaluator::evaluateKBNK(const Position &pos, Color lhs) const {
    constexpr i32 LONE_KING_BONUS_DS[] {
            0, 1, 2, 3, 4, 5, 6, 7,
            1, 2, 3, 4, 5, 6, 7, 6,
            2, 3, 4, 5, 6, 7, 6, 5,
            3, 4, 5, 6, 7, 6, 5, 4,
            4, 5, 6, 7, 6, 5, 4, 3,
            5, 6, 7, 6, 5, 4, 3, 2,
            6, 7, 6, 5, 4, 3, 2, 1,
            7, 6, 5, 4, 3, 2, 1, 0,
    };
    constexpr i32 LONE_KING_BONUS_LS[] {
            7, 6, 5, 4, 3, 2, 1, 0,
            6, 7, 6, 5, 4, 3, 2, 1,
            5, 6, 7, 6, 5, 4, 3, 2,
            4, 5, 6, 7, 6, 5, 4, 3,
            3, 4, 5, 6, 7, 6, 5, 4,
            2, 3, 4, 5, 6, 7, 6, 5,
            1, 2, 3, 4, 5, 6, 7, 6,
            0, 1, 2, 3, 4, 5, 6, 7,
    };

    i32 base = m_Weights->material[PT_BISHOP].get(0) +
            m_Weights->material[PT_KNIGHT].get(0) +
            m_Weights->material[PT_PAWN].get(0) / 2;

    Square ourBishop = *pos.getBitboard(Piece(lhs, PT_BISHOP)).begin();
    Square theirKing = pos.getKingSquare(getOppositeColor(lhs));

    i32 theirKingBonus;
    if (bbs::LIGHT_SQUARES.contains(ourBishop)) {
        theirKingBonus = LONE_KING_BONUS_LS[theirKing];
    }
    else {
        theirKingBonus = LONE_KING_BONUS_DS[theirKing];
    }

    return base - theirKingBonus * 50;
}



}