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

namespace staticanalysis {

/**
 * Returns true if a given move in a position yields a static exchange
 * evaluation value higher than the specified threshold.
 *
 * For more information: https://www.chessprogramming.org/Static_Exchange_Evaluation
 */
bool hasGoodSEE(const Position& pos, Move move, int threshold = 0);

int guardValue(const Position& pos, Square s, Color pov);

/**
 * Returns true if a pawn on the given square is a passer.
 * Does not check if the piece at the square is actually a pawn.
 */
bool isPassedPawn(const Position& pos, Square pawnSquare);

/**
 * Returns the bitboard for all pieces of the specified type that are currently placed in outposts.
 * For this, we consider an 'outpost' to be every square that cannot be contested
 * by an enemy pawn and is defended by at least one allied pawn.
 */
Bitboard getPieceOutposts(const Position& pos, Piece p);

/**
 * Returns the bitboard for all passed pawns of the specified color.
 * Passed pawns are pawns that have no opposing pawns on their file or adjacent files.
 */
Bitboard getPassedPawns(const Position& pos, Color c);

/**
 * Returns the bitboard for all pawns of the specified color that have other pawns
 * on neighboring files.
 */
Bitboard getConnectedPawns(const Position& pos, Color c);

/**
 * Returns the bitboard for all passed pawns of the specified color that have other passed
 * pawns on neighboring files.
 */
Bitboard getConnectedPassers(const Position& pos, Color c);

/**
 * Returns the bitboard for all pawns of the specified color that are blocking another
 * pawn of the same color in their file (aka "doubled/tripled/..." pawns)
 */
Bitboard getBlockingPawns(const Position& pos, Color c);

/**
 * Returns the bitboard for all backward pawns of the specified color.
 */
Bitboard getBackwardPawns(const Position& pos, Color c);

/**
 * Returns all squares in the position that are defended by a piece of the specified color.
 * A 'highestValueDefenderType' can be passed to stop considering stronger pieces as defenders.
 * Eg: highestValueDefenderType == PT_ROOK will not return squares that are defended by queens or kings.
 */
Bitboard getDefendedSquares(const Position& pos, Color c, PieceType highestValueDefenderType = PT_KING);

//
// The following getXXAttackers functions return the bitboard of all pieces that
// are currently attacking a specific square in a given position.
//

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

inline KingsDistribution getKingsDistribution(const Position& pos, Color a = CL_WHITE) {
    Color b = getOppositeColor(a);

    Square aKingSquare = pos.getKingSquare(a);
    Square bKingSquare = pos.getKingSquare(b);

    Bitboard kingSide = bbs::getBoardSide(SIDE_KING);
    Bitboard queenSide = bbs::getBoardSide(SIDE_QUEEN);

    if (kingSide.contains(aKingSquare)) {
        // Side A on King Side
        return kingSide.contains(bKingSquare) ? KD_KK : KD_KQ;
    }
    // Side A on Queen Side
    return queenSide.contains(bKingSquare) ? KD_QQ : KD_QK;
}

} // staticanalysis

} // lunachess

#endif // LUNA_POSUTILS_H