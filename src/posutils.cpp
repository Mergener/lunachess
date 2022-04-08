#include "posutils.h"

namespace lunachess::posutils {

static Bitboard getAllAttackersSEE(const Position& pos, Square s) {
    Bitboard atks = 0;
    Bitboard newAtks = 0;
    Bitboard occ = pos.getCompositeBitboard();
    do {
        atks = newAtks;
        newAtks = getAttackers(pos, s, CL_WHITE, occ);
        newAtks |= getAttackers(pos, s, CL_BLACK, occ);
        occ &= ~newAtks;
    } while (newAtks != atks);

    return atks;
}

static bool isAttackingDirectly(Bitboard occ, Square atkSquare, Square s) {
    Bitboard between = bbs::getSquaresBetween(atkSquare, s);
    bool ret = (between & occ) == 0;
    return ret;
}

static Square getLeastValueAttacker(const Position& pos, Bitboard atks, Bitboard occ, Square destSqr, Color c) {
    // Loop from least value piece type to highest value
    for (PieceType pt = PT_PAWN; pt < PT_COUNT; ++pt) {
        Piece p = Piece(c, pt);
        Bitboard bb = atks & pos.getBitboard(p);
        if (bb == 0) {
            // No pieces of this type remaining in attackers bitboard
            continue;
        }

        // We have attacking pieces, but must consider if they're not currently being
        // blocked (xrayers)
        while (bb != 0) {
            Square s = *bb.begin();
            if (isAttackingDirectly(occ, s, destSqr)) {
                // Nothing between the attacker and the dest square
                return s;
            }
            // Something in between, discard this square
            bb.remove(s);
        }
    }
    // No attackers found
    return SQ_INVALID;
}

bool hasGoodSEE(const Position& pos, Move move, int threshold) {
    Bitboard occ = pos.getCompositeBitboard();
    Square targetSqr = move.getDest();
    Bitboard atks = getAllAttackersSEE(pos, targetSqr);

    // Initial totalGain is the value of the piece at the target square
    // being captured.
    int totalGain = getPiecePointValue(move.getDestPiece().getType());

    // The type of the piece that is now standing on the
    // target square -- the trophy piece.
    PieceType trophyPT = move.getSourcePiece().getType();
    int nextGain = getPiecePointValue(trophyPT);

    // The source square of the trophy piece
    Square atkSqr = move.getSource();

    // Their color
    Color c = move.getSourcePiece().getColor();

    // Since the first capture is made by 'us', sign starts at 1.
    // When simulating further captures, this sign is toggled and multiplied
    // by the point value to simulate the losses of material.
    int sign = 1;

    atks.remove(atkSqr);
    occ.remove(atkSqr);

    while (atks != 0) {
        // Simulating next player's turn
        c = getOppositeColor(c);
        sign *= -1;

        // Find least value attacker from current player
        atkSqr = getLeastValueAttacker(pos, atks, occ, targetSqr, c);
        if (atkSqr == SQ_INVALID) {
            break;
        }
        atks.remove(atkSqr);
        occ.remove(atkSqr);

        // Suppose current player captures
        totalGain += sign * nextGain;

        // Prepare next attacker
        trophyPT = pos.getPieceAt(atkSqr).getType();
        nextGain = getPiecePointValue(trophyPT);

        if (sign > 0) {
            if (totalGain - nextGain >= threshold) {
                // Even if they recaptured our piece, we could
                // stop the capture chain and still be in a winning
                // exchange.
                return true;
            }
        }
        else {
            if (totalGain + nextGain < threshold) {
                // Even if we recapture our opponent piece, they
                // could stop the capture chain and we would still
                // be in a losing exchange.
                return false;
            }
        }
    }

    return totalGain >= threshold;
}

} // lunachess::posutils