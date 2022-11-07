#include "neuraleval.h"

namespace lunachess::ai::neural {

void NNInputs::fromPosition(const Position& pos) {
    // Zero-out everything first
    utils::zero(*this);

    // Fill piece square tables
    for (Color c: { CL_WHITE, CL_BLACK }) {
        for (PieceType pt: { PT_PAWN, PT_KNIGHT, PT_BISHOP, PT_ROOK, PT_QUEEN, PT_KING }) {
            Piece p     = Piece(c, pt);
            Bitboard bb = pos.getBitboard(p);

            for (Square s: bb) {
                psqt[c][pt - 1][s] = 1;
            }
        }
    }

    castleRights[CL_WHITE][SIDE_KING]  = pos.getCastleRights(CL_WHITE, SIDE_KING) ? 1 : 0;
    castleRights[CL_WHITE][SIDE_QUEEN] = pos.getCastleRights(CL_WHITE, SIDE_QUEEN) ? 1 : 0;
    castleRights[CL_BLACK][SIDE_KING]  = pos.getCastleRights(CL_BLACK, SIDE_KING) ? 1 : 0;
    castleRights[CL_BLACK][SIDE_QUEEN] = pos.getCastleRights(CL_BLACK, SIDE_QUEEN) ? 1 : 0;

    colorToMove = pos.getColorToMove();
}

void NNInputs::updateMakeMove(Move move) {
    // Fetch required values
    Square src      = move.getSource();
    Square dst      = move.getDest();
    Piece srcPiece  = move.getSourcePiece();
    Piece dstPiece  = move.getDestPiece();
    PieceType srcPt = srcPiece.getType();
    PieceType dstPt = dstPiece.getType();
    Color srcColor  = srcPiece.getColor();
    Color dstColor  = dstPiece.getColor();

    // Update color to move
    colorToMove = dstColor;

    // Update PSQTs
    psqt[srcColor][srcPt - 1][src] = 0; // clear source piece
    psqt[dstColor][dstPt - 1][dst] = 0; // clear prev dest piece
    if (move.is<MTM_PROMOTION>()) {
        // We have a promotion, dest square will have
        // a promotion piece.
        PieceType promPT = move.getPromotionPieceType();
        psqt[srcColor][promPT - 1][dst] = 1;
        return;
    }

    // No promotions, dest square will receive
    // the source piece type.
    psqt[srcColor][srcPt - 1][dst] = 1;

    switch (move.getType()) {
        case MT_EN_PASSANT_CAPTURE:
            psqt[dstColor][PT_PAWN - 1][getEnPassantVictimSquare(dst)] = 0;
            break;

        case MT_CASTLES_SHORT:
            psqt[srcColor][PT_ROOK - 1][getCastleRookSrcSquare(srcColor, SIDE_KING)] = 0;
            psqt[srcColor][PT_ROOK - 1][getCastleRookDestSquare(srcColor, SIDE_KING)] = 1;
            break;

        case MT_CASTLES_LONG:
            psqt[srcColor][PT_ROOK - 1][getCastleRookSrcSquare(srcColor, SIDE_QUEEN)] = 0;
            psqt[srcColor][PT_ROOK - 1][getCastleRookDestSquare(srcColor, SIDE_QUEEN)] = 1;
            break;

        case MT_NORMAL:
        case MT_SIMPLE_CAPTURE:
        case MT_DOUBLE_PUSH:
        case MT_PROMOTION_CAPTURE:
        case MT_SIMPLE_PROMOTION:
            // Do nothing
            break;
        default:
            LUNA_ASSERT(false, "Move type not implemented.");
            break;
    }
}

void NNInputs::updateUndoMove(Move move) {
// Fetch required values
    Square src      = move.getSource();
    Square dst      = move.getDest();
    Piece srcPiece  = move.getSourcePiece();
    Piece dstPiece  = move.getDestPiece();
    PieceType srcPt = srcPiece.getType();
    PieceType dstPt = dstPiece.getType();
    Color srcColor  = srcPiece.getColor();
    Color dstColor  = dstPiece.getColor();

    // Update color to move
    colorToMove = srcColor;

    // Update PSQTs
    psqt[srcColor][srcPt - 1][src] = 1; // set source piece
    psqt[dstColor][dstPt - 1][dst] = 1; // set prev dest piece
    if (move.is<MTM_PROMOTION>()) {
        // We had a promotion, clear dest square for promPt
        PieceType promPT = move.getPromotionPieceType();
        psqt[srcColor][promPT - 1][dst] = 0;
        return;
    }

    // No promotions, dest square for srcPT must be
    // unset
    psqt[srcColor][srcPt - 1][dst] = 0;

    switch (move.getType()) {
        case MT_EN_PASSANT_CAPTURE:
            psqt[dstColor][PT_PAWN - 1][getEnPassantVictimSquare(dst)] = 1;
            break;

        case MT_CASTLES_SHORT:
            psqt[srcColor][PT_ROOK - 1][getCastleRookSrcSquare(srcColor, SIDE_KING)]  = 1;
            psqt[srcColor][PT_ROOK - 1][getCastleRookDestSquare(srcColor, SIDE_KING)] = 0;
            break;

        case MT_CASTLES_LONG:
            psqt[srcColor][PT_ROOK - 1][getCastleRookSrcSquare(srcColor, SIDE_QUEEN)]  = 1;
            psqt[srcColor][PT_ROOK - 1][getCastleRookDestSquare(srcColor, SIDE_QUEEN)] = 0;
            break;

        case MT_NORMAL:
        case MT_SIMPLE_CAPTURE:
        case MT_DOUBLE_PUSH:
        case MT_PROMOTION_CAPTURE:
        case MT_SIMPLE_PROMOTION:
            // Do nothing
            break;
        default:
            LUNA_ASSERT(false, "Move type not implemented.");
            break;
    }
}

void NNInputs::updateMakeNullMove() {
    colorToMove = !colorToMove;
}

void NNInputs::updateUndoNullMove() {
    colorToMove = !colorToMove;
}

int EvalNN::evaluate(const i32* arr) {
    W1Layer::OutputArray w1out;
    W2Layer::OutputArray w2out;
    W3Layer::OutputArray w3out;
    W4Layer::OutputArray w4out;

    w1.propagate(arr, w1out.data());
    w2.propagate(w1out.data(), w2out.data());
    w3.propagate(w2out.data(), w3out.data());
    w4.propagate(w3out.data(), w4out.data());

    return w4out[0];
}

const std::shared_ptr<EvalNN> g_DefaultNN = std::make_shared<EvalNN>();

};

