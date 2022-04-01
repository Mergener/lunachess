#ifndef LUNA_POSUTILS_H
#define LUNA_POSUTILS_H

#include "position.h"

namespace lunachess {

enum FileState {

    /** No pawns from either color on the file */
    FS_OPEN,

    /** Pawns from both colors on the file */
    FS_CLOSED,

    /** One of the colors has pawns on the file */
    FS_SEMIOPEN
};

namespace posutils {

inline Bitboard getPawnAttackers(const Position &pos, Square s, Color c) {
    Bitboard pawns = pos.getBitboard(Piece(c, PT_PAWN));
    Bitboard squareBB = BIT(s);
    Direction dirLeft = getPawnLeftCaptureDir(c);
    Direction dirRight = getPawnRightCaptureDir(c);
    Bitboard mask = squareBB.shifted(-dirLeft) | squareBB.shifted(-dirRight);

    return pawns & mask;
}

inline Bitboard getKnightAttackers(const Position &pos, Square s, Color c) {
    Bitboard knights = pos.getBitboard(Piece(c, PT_KNIGHT));
    return knights & bbs::getKnightAttacks(s);
}

inline Bitboard getBishopAttackers(const Position &pos, Square s, Color c, Bitboard occ) {
    Bitboard bishops = pos.getBitboard(Piece(c, PT_BISHOP));
    return bishops & bbs::getBishopAttacks(s, occ);
}

inline Bitboard getBishopAttackers(const Position &pos, Square s, Color c) {
    return getBishopAttackers(pos, s, c, pos.getCompositeBitboard());
}

inline Bitboard getRookAttackers(const Position &pos, Square s, Color c, Bitboard occ) {
    Bitboard rooks = pos.getBitboard(Piece(c, PT_ROOK));
    return rooks & bbs::getRookAttacks(s, occ);
}

inline Bitboard getRookAttackers(const Position &pos, Square s, Color c) {
    return getRookAttackers(pos, s, c, pos.getCompositeBitboard());
}

inline Bitboard getQueenAttackers(const Position &pos, Square s, Color c, Bitboard occ) {
    Bitboard queens = pos.getBitboard(Piece(c, PT_QUEEN));
    return queens & bbs::getQueenAttacks(s, occ);
}

inline Bitboard getQueenAttackers(const Position &pos, Square s, Color c) {
    return getQueenAttackers(pos, s, c, pos.getCompositeBitboard());
}

inline Bitboard getKingAttackers(const Position &pos, Square s, Color c) {
    Bitboard kings = pos.getBitboard(Piece(c, PT_KING));
    return kings & bbs::getKingAttacks(s);
}

inline Bitboard getAttackers(const Position &pos, Square s, Color c, Bitboard occ) {
    return getPawnAttackers(pos, s, c)
           | getKnightAttackers(pos, s, c)
           | getBishopAttackers(pos, s, c, occ)
           | getRookAttackers(pos, s, c, occ)
           | getQueenAttackers(pos, s, c, occ)
           | getKingAttackers(pos, s, c);
}

inline Bitboard getAttackers(const Position &pos, Square s, Color c) {
    return getAttackers(pos, s, c, pos.getCompositeBitboard());
}

inline FileState getFileState(const Position& pos, BoardFile f) {
    Bitboard fileBB = bbs::getFileBitboard(f);

    Bitboard wp = pos.getBitboard(WHITE_PAWN) & fileBB;
    Bitboard bp = pos.getBitboard(BLACK_PAWN) & fileBB;

    if (wp == 0 && bp == 0) {
        // No pawns on the file
        return FS_OPEN;
    }
    if (wp != 0 && bp != 0) {
        // Pawns on both sides on the file
        return FS_CLOSED;
    }
    // A side has no pawns on the file, while the other has.
    return FS_SEMIOPEN;
}

} // posutils

} // lunachess

#endif // LUNA_POSUTILS_H