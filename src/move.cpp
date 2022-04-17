#include "move.h"

#include <new>

#include "position.h"

namespace lunachess {

Move::Move(const Position& pos, std::string_view sv) {
    if (sv.size() < 4) {
        m_Data = MOVE_INVALID.m_Data;
        return;
    }

    Square src = getSquare(sv.substr(0, 2));
    Square dst = getSquare(sv.substr(2, 2));

    PieceType promTarget;
    if (sv.size() >= 5) {
        promTarget = Piece::fromIdentifier(sv[4]).getType();
    }
    else {
        promTarget = PT_NONE;
    }

    new (this) Move(pos, src, dst, promTarget);
}

Move::Move(const Position& pos, Square src, Square dst, PieceType promotionPieceType)
    : Move(src, dst, pos.getPieceAt(src), pos.getPieceAt(dst), MT_NORMAL, promotionPieceType) {

    MoveType type = MT_NORMAL;

    if (getSourcePiece().getType() == PT_PAWN) {
        // Pawn moves
        if (getFile(getSource()) == getFile(getDest())) {
            // Pawn push
            int delta = std::abs(getRank(getDest()) - getRank(getSource()));
            if (delta == 2) {
                type = MT_DOUBLE_PUSH;
            }
            else if (getPromotionPiece() != PT_NONE) {
                type = MT_SIMPLE_PROMOTION;
            }
            else {
                type = MT_NORMAL;
            }
        }
        else {
            // A pawn capture. Check for capture types.
            if (getDestPiece() == PIECE_NONE) {
                // No piece at the destination square, en passant capture.
                type = MT_EN_PASSANT_CAPTURE;
            }
            else if (getPromotionPiece() == PT_NONE) {
                type = MT_SIMPLE_CAPTURE;
            }
            else {
                type = MT_PROMOTION_CAPTURE;
            }
        }
    }
    else if (getSourcePiece().getType() == PT_KING) {
        // Check if castles
        int delta = getFile(getDest()) - getFile(getSource());
        if (delta > 1) {
            // Source piece is a king and is more than 1 file away from its original square,
            // it is castles move.
            type = MT_CASTLES_SHORT;
        }
        else if (delta < -1) {
            type = MT_CASTLES_LONG;
        }
    }
    else if (getDestPiece() != PIECE_NONE) {
        // Certain capture
        type = MT_SIMPLE_CAPTURE;
    }

    m_Data |= (type & BITMASK(6)) << 23;
}

std::ostream& operator<<(std::ostream& stream, Move m) {

    stream << getSquareName(m.getSource());
    stream << getSquareName(m.getDest());

    if (m.is<MTM_PROMOTION>()) {
        Piece p = Piece(CL_BLACK, m.getPromotionPiece());
        stream << p.getIdentifier();
    }

    return stream;
}

}