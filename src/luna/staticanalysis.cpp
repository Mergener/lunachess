#include "staticanalysis.h"

#include <cstring>

#include "bitboard.h"

namespace lunachess::staticanalysis {

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
    if (move.getDestPiece().getType() != PT_NONE &&
        getPiecePointValue(move.getDestPiece().getType()) > getPiecePointValue(move.getSourcePiece().getType())) {
        return true;
    }

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

int guardValue(const Position& pos, Square s, Color us) {
    constexpr int CAPTURE_STRENGTH[] = { 0, 9, 6, 5, 2, 1, 1 };

    bool attacked = false;
    bool defended = false;
    int gv = 0;
    Bitboard atks = getAllAttackersSEE(pos, s);

    for (Square atk : atks) {
        Piece p = pos.getPieceAt(atk);
        if (p.getColor() == us) {
            attacked = true;
            gv += CAPTURE_STRENGTH[p.getType()];
        }
        else {
            defended = true;
            gv -= CAPTURE_STRENGTH[p.getType()];
        }
    }
    if (attacked && defended) {
        Piece p = pos.getPieceAt(s);
        if (p.getColor() != us) {
            gv -= CAPTURE_STRENGTH[p.getType()];
        }
    }
    return gv;
}

bool isPassedPawn(const Position& pos, Square s) {
    Color c = pos.getPieceAt(s).getColor();
    Bitboard theirPawns = pos.getBitboard(Piece(getOppositeColor(c), PT_PAWN));
    Bitboard cantHaveEnemyPawnsBB =
            bbs::getFileContestantsBitboard(s, c) | bbs::getPasserBlockerBitboard(s, c);

    return (cantHaveEnemyPawnsBB & theirPawns) == 0;
}

Bitboard getPassedPawns(const Position& pos, Color c) {
    Bitboard pawns = pos.getBitboard(Piece(c, PT_PAWN));
    Bitboard theirPawns = pos.getBitboard(Piece(getOppositeColor(c), PT_PAWN));
    Bitboard bb = 0;

    for (Square s: pawns) {
        Bitboard cantHaveEnemyPawnsBB =
            bbs::getFileContestantsBitboard(s, c) | bbs::getPasserBlockerBitboard(s, c);

        if ((cantHaveEnemyPawnsBB & theirPawns) == 0) {
            // No nearby file contestants and opposing blockers.
            // Pawn is a passer!
            bb.add(s);
        }
    }

    return bb;
}

Bitboard getPieceOutposts(const Position& pos, Piece p) {
    Bitboard pieceBB = pos.getBitboard(p);
    Bitboard bb = 0;
    Color us = p.getColor();
    Bitboard theirPawns = pos.getBitboard(Piece(getOppositeColor(us), PT_PAWN));
    Bitboard ourPawnAttacks = pos.getAttacks(us, PT_PAWN);

    for (auto s: pieceBB) {
        Bitboard constestantsBB = bbs::getFileContestantsBitboard(s, us);
        if ((constestantsBB & theirPawns) == 0 && ourPawnAttacks.contains(s)) {
            bb.add(s);
        }
    }

    return bb;
}

template <bool PASSERS>
static Bitboard getConnectedPawnsOrPassers(const Position& pos, Color us) {
    Bitboard pawns;
    if constexpr (PASSERS) {
        pawns = getPassedPawns(pos, us);
    }
    else {
        pawns = pos.getBitboard(Piece(us, PT_PAWN));
    }
    Bitboard bb = 0;
    if (bbs::getFileBitboard(FL_B) & pawns) {
        bb |= bbs::getFileBitboard(FL_A) & pawns;
    }

    for (BoardFile f = FL_B; f < FL_COUNT - 1; ++f) {
        Bitboard thisFilePawns = bbs::getFileBitboard(f) & pawns;

        Bitboard lastFilePawns = bbs::getFileBitboard(f - 1) & pawns;
        Bitboard nextFilePawns = bbs::getFileBitboard(f + 1) & pawns;
        if (nextFilePawns || lastFilePawns) {
            bb |= thisFilePawns;
        }
    }

    if (bbs::getFileBitboard(FL_G) & pawns) {
        bb |= bbs::getFileBitboard(FL_H) & pawns;
    }

    return bb;
}

Bitboard getConnectedPawns(const Position& pos, Color us) {
    return getConnectedPawnsOrPassers<false>(pos, us);
}

Bitboard getConnectedPassers(const Position& pos, Color us) {
    return getConnectedPawnsOrPassers<true>(pos, us);
}

Bitboard getBlockingPawns(const Position& pos, Color c) {
    Bitboard bb = 0;
    Bitboard pawns = pos.getBitboard(Piece(c, PT_PAWN));

    // Evaluate blocking pawns
    for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
        Bitboard filePawns = bbs::getFileBitboard(f) & pawns;

        // The pawn at the bottom of the stack is not a blocker.
        // Remove it.
        if (c == CL_WHITE) {
            filePawns.remove(*filePawns.begin());
        }
        else {
            filePawns.remove(*filePawns.rbegin());
        }

        bb |= filePawns;
    }

    return bb;
}

Bitboard getBackwardPawns(const Position& pos, Color us) {
    Color them = getOppositeColor(us);
    Bitboard ourPawns = pos.getBitboard(Piece(us, PT_PAWN));
    Bitboard bb = 0;
    Direction pawnStepDir = getPawnStepDir(us);

    for (auto s: ourPawns) {
        Square nextSquare = s + pawnStepDir;

        if (!pos.getAttacks(them, PT_PAWN).contains(nextSquare)) {
            // Opponent is not attacking the square in front of the pawn with their pawns
            continue;
        }

        Bitboard supportingPawns = ourPawns & bbs::getFileContestantsBitboard(s, them);
        if (supportingPawns != 0) {
            // We have a pawn behind this pawn that is already supporting this pawn or can be pushed
            // to do it.
            continue;
        }

        if (pos.getPieceAt(nextSquare).getType() == PT_PAWN) {
            // Square above is occupied by a pawn (from either side)
            continue;
        }

        bb.add(s);
    }

    return bb;
}

Bitboard getDefendedSquares(const Position& pos, Color c, PieceType highestValueDefenderType) {
    Bitboard bb = 0;
    switch (highestValueDefenderType) {
        default: return pos.getAttacks(c, PT_NONE);

        case PT_KING:   bb |= pos.getAttacks(c, PT_KING);
        case PT_QUEEN:  bb |= pos.getAttacks(c, PT_QUEEN);
        case PT_ROOK:   bb |= pos.getAttacks(c, PT_ROOK);
        case PT_KNIGHT: bb |= pos.getAttacks(c, PT_KNIGHT);
        case PT_BISHOP: bb |= pos.getAttacks(c, PT_BISHOP);
        case PT_PAWN:   bb |= pos.getAttacks(c, PT_PAWN);
    }
    return bb;
}

} // lunachess::staticanalysis