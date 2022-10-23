//#include "neuraleval.h"
//
//#include <algorithm>
//#include <cstring>
//#include <mmintrin.h>
//#include <random>
//
//namespace lunachess::ai::neural {
//
//static std::mt19937_64 s_Random (std::random_device{}());
//
//int NeuralEvaluator::evaluate(const NeuralInputs& inputs) const {
//    int neuralOut = static_cast<int>(m_Network->evaluate(inputs));
//    return neuralOut;
//}
//
//int NeuralEvaluator::evaluate(const Position& pos) const {
//    NeuralInputs inputs(pos);
//
//    Color us = pos.getColorToMove();
//    Color them = getOppositeColor(us);
//
//    return evaluate(inputs)
//        + pos.getBitboard(Piece(us, PT_PAWN)).count() * 100
//        + pos.getBitboard(Piece(us, PT_KNIGHT)).count() * 310
//        + pos.getBitboard(Piece(us, PT_BISHOP)).count() * 320
//        + pos.getBitboard(Piece(us, PT_ROOK)).count() * 500
//        + pos.getBitboard(Piece(us, PT_QUEEN)).count() * 900
//        - pos.getBitboard(Piece(them, PT_PAWN)).count() * 100
//        - pos.getBitboard(Piece(them, PT_KNIGHT)).count() * 310
//        - pos.getBitboard(Piece(them, PT_BISHOP)).count() * 320
//        - pos.getBitboard(Piece(them, PT_ROOK)).count() * 500
//        - pos.getBitboard(Piece(them, PT_QUEEN)).count() * 900;
//}
//
//NeuralInputs::NeuralInputs(const Position& pos, Color us) {
//    zeroAll();
//
//    // Piece
//    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
//        for (PieceType pt = PT_PAWN; pt < PT_COUNT; ++pt) {
//            Piece p = Piece(c, pt);
//            Bitboard bb = pos.getBitboard(p);
//
//            for (Square s: bb) {
//                int arrIdx = c == us ? 0 : 1;
//                Square sqIdx = us == CL_WHITE ? s : getSquare(FL_COUNT - getFile(s) - 1, getRank(s));
//                pieceMaps[arrIdx][pt - 1][sqIdx] = 1.0f;
//            }
//        }
//    }
//
//    // Castling rights
//    if (pos.getCastleRights(us, SIDE_KING)) {
//        weCanCastleShort = 1.0f;
//    }
//    if (pos.getCastleRights(us, SIDE_QUEEN)) {
//        weCanCastleLong = 1.0f;
//    }
//
//    if (pos.getCastleRights(getOppositeColor(us), SIDE_KING)) {
//        theyCanCastleShort = 1.0f;
//    }
//    if (pos.getCastleRights(getOppositeColor(us), SIDE_QUEEN)) {
//        theyCanCastleLong = 1.0f;
//    }
//}
//
//};
//
