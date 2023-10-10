#include "nnue.h"

#include <cstring>

namespace lunachess::ai::nnue {

static size_t getFeatureIndex(Square s, Color ctm, Piece piece) {
    constexpr size_t N_PIECES  = 6;
    constexpr size_t N_SQUARES = 64;

    size_t colorIdx  = piece.getColor();
    size_t pieceIdx  = piece.getType();
    size_t squareIdx = ctm == CL_BLACK ? mirrorVertically(s) : s;

    return  (N_PIECES * N_SQUARES) * colorIdx +
            (N_SQUARES) * pieceIdx +
            squareIdx;
}

void Accumulator::refresh(const Position& pos,
                          const i16* weights,
                          const i16* biases) {
    std::memset(featuresSum.data(), 0, sizeof(featuresSum));

    for (Color c: { CL_WHITE, CL_BLACK }) {
        // Generate features array
        FeaturesArray features = {};

        for (PieceType pt: { PT_PAWN, PT_KNIGHT, PT_BISHOP, PT_ROOK, PT_QUEEN, PT_KING }) {
            Bitboard bb = pos.getBitboard(Piece(c, pt));
            for (Square s: bb) {
                Piece p            = Piece(c, pt);
                size_t index       = getFeatureIndex(s, c, p);
                features[index] = 1;
            }
        }

        // Compute affine transformation
        affineTransform<N_FEATURES, L1_SIZE>(features.data(),
                                             featuresSum[c].data(),
                                             weights, biases);
    }
}

static void updateAccumulatorSquare(Accumulator& accum,
                                     size_t featureArray,
                                     Square square,
                                     PieceType previousPieceType,
                                     PieceType newPieceType,
                                     const i16* weights) {
    i16 sign = newValue == 1 ? 1 : -1;
    for (Color c: { CL_WHITE, CL_BLACK }) {
        for (size_t i = 0; i < L1_SIZE; ++i) {
            i32 weightValue = static_cast<i32>(weights[i * N_FEATURES + featureIndex]);
            accum.featuresSum[c][i] += sign * weightValue;
        }
    }
}

void Accumulator::update(Move move,
                         const i16* weights,
                         const i16* biases) {

}


}