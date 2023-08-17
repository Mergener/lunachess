#include "move.h"

#include <new>

#include "bitboard.h"
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

        if (getDestPiece() != PIECE_NONE) {
            type = MT_SIMPLE_CAPTURE;
        }
    }
    else if (getDestPiece() != PIECE_NONE) {
        // Certain capture
        type = MT_SIMPLE_CAPTURE;
    }

    m_Data |= (type & BITMASK(6)) << 23;
}

std::ostream& operator<<(std::ostream& stream, Move m) {

    if (m == MOVE_INVALID) {
        stream << "null";
        return stream;
    }

    stream << getSquareName(m.getSource());
    stream << getSquareName(m.getDest());

    if (m.is<MTM_PROMOTION>()) {
        Piece p = Piece(CL_BLACK, m.getPromotionPiece());
        stream << p.getIdentifier();
    }

    return stream;
}

std::string Move::toAlgebraic(const Position& pos) const {
    if (getType() == MT_CASTLES_LONG) {
        return "O-O-O";
    }
    if (getType() == MT_CASTLES_SHORT) {
        return "O-O";
    }

    char ret[32];
    int idx = 0;

    Piece srcPiece = getSourcePiece();
    Square src = getSource();
    Square dest = getDest();

    // If moving piece is not a pawn, add its identifier letter
    // Also handle captures here, since pawns and pieces behave
    // differently.
    if (srcPiece.getType() != PT_PAWN) {
        // Not a pawn
        ret[idx++] = std::toupper(srcPiece.getIdentifier());

        // Check if another piece of the same type can move to the
        // specified square
        Bitboard atks = bbs::getPieceAttacks(dest, 0, srcPiece) & pos.getBitboard(srcPiece);
        if (atks.count() > 1) {
            // We have another piece that could've done the same move as our.
            // Try to differentiate it with file name.
            BoardFile srcFile = getFile(src);
            Bitboard fileBB = bbs::getFileBitboard(srcFile) & atks;

            if (fileBB.count() == 1) {
                // Other piece is on another file, use the file to differentiate.
                ret[idx++] = std::tolower(getFileIdentifier(srcFile));
            }
            else {
                // There is another piece on the same file as the sourcePiece.
                // Use the rank to differentiate.
                ret[idx++] = std::tolower(getRankIdentifier(getRank(src)));
            }
        }

        if (is<MTM_CAPTURE>()) {
            ret[idx++] = 'x';
        }
    }
    else {
        // srcPiece is a pawn
        if (is<MTM_CAPTURE>()) {
            ret[idx++] = std::tolower(getFileIdentifier(getFile(src)));
            ret[idx++] = 'x';
        }
    }

    // Add destination square
    ret[idx++] = std::tolower(getFileIdentifier(getFile(dest)));
    ret[idx++] = std::tolower(getRankIdentifier(getRank(dest)));

    if (is<MTM_PROMOTION>()) {
        ret[idx++] = '=';
        ret[idx++] = Piece(CL_WHITE, getPromotionPiece()).getIdentifier();
    }

    ret[idx] = '\0';
    return std::string(ret);
}

}