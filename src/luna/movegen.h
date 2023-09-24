#ifndef LUNA_MOVEGEN_H
#define LUNA_MOVEGEN_H

#include "position.h"

namespace lunachess {

using MoveList = StaticList<Move, 256>;

namespace movegen {

namespace utils {

template<Color C, MoveTypeMask ALLOWED_FLAGS>
void generatePawnMoves(const Position &pos, MoveList &ml) {
    constexpr Piece SRC_PIECE = Piece(C, PT_PAWN);

    // Get movement constants
    constexpr Direction STEP_DIR = PAWN_STEP_DIR<C>;
    constexpr Direction LEFT_CAPT_DIR = PAWN_CAPT_LEFT_DIR<C>;
    constexpr Direction RIGHT_CAPT_DIR = PAWN_CAPT_RIGHT_DIR<C>;
    constexpr BoardRank PROM_RANK = PAWN_PROMOTION_RANK<C>;
    constexpr BoardRank DOUBLE_PUSH_RANK = C == CL_WHITE ? RANK_4 : RANK_5;

    // Check allowed move types
    constexpr bool GEN_SIMPLE_CAPTURES = ALLOWED_FLAGS & BIT(MT_SIMPLE_CAPTURE);
    constexpr bool GEN_SIMPLE_PROMOTIONS = ALLOWED_FLAGS & BIT(MT_SIMPLE_PROMOTION);
    constexpr bool GEN_PROM_CAPT = ALLOWED_FLAGS & BIT(MT_PROMOTION_CAPTURE);
    constexpr bool GEN_SINGLE_PUSHES = ALLOWED_FLAGS & BIT(MT_NORMAL);
    constexpr bool GEN_DOUBLE_PUSHES = ALLOWED_FLAGS & BIT(MT_DOUBLE_PUSH);
    constexpr bool GEN_EP = ALLOWED_FLAGS & BIT(MT_EN_PASSANT_CAPTURE);

    Bitboard occ     = pos.getCompositeBitboard();
    Bitboard theirBB = pos.getBitboard(Piece(getOppositeColor(C), PT_NONE));
    Bitboard pawns   = pos.getBitboard(Piece(C, PT_PAWN));

    // Generate capture-promotions
    if constexpr (GEN_PROM_CAPT) {
        Bitboard promRankBB = bbs::getRankBitboard(PROM_RANK);

        // Left captures
        Bitboard leftAttacks = pawns.shifted<LEFT_CAPT_DIR>() & theirBB;

        // Include only promotion rank
        leftAttacks &= promRankBB;

        for (auto s: leftAttacks) {
            Square src = s - LEFT_CAPT_DIR;

            for (PieceType pt = PT_QUEEN; pt >= PT_KNIGHT; --pt) {
                Move move(src, s, SRC_PIECE, pos.getPieceAt(s), MT_PROMOTION_CAPTURE, pt);
                ml.add(move);
            }
        }

        // Right captures
        Bitboard rightAttacks = pawns.shifted<RIGHT_CAPT_DIR>() & theirBB;

        // Include only promotion rank
        rightAttacks &= promRankBB;

        for (auto s: rightAttacks) {
            Square src = s - RIGHT_CAPT_DIR;

            for (PieceType pt = PT_QUEEN; pt >= PT_KNIGHT; --pt) {
                Move move(src, s, SRC_PIECE, pos.getPieceAt(s), MT_PROMOTION_CAPTURE, pt);
                ml.add(move);
            }
        }
    }

    // Generate non-promotion and non-en-passant captures
    if constexpr (GEN_SIMPLE_CAPTURES) {
        Bitboard promRankBB = bbs::getRankBitboard(PROM_RANK);

        // Left captures
        Bitboard leftAttacks = pawns.shifted<LEFT_CAPT_DIR>() & theirBB;

        // Exclude promotion rank
        leftAttacks &= ~promRankBB;

        for (auto s: leftAttacks) {
            Square src = s - LEFT_CAPT_DIR;
            Move move(src, s, SRC_PIECE, pos.getPieceAt(s), MT_SIMPLE_CAPTURE);
            ml.add(move);
        }

        // Right captures
        Bitboard rightAttacks = pawns.shifted<RIGHT_CAPT_DIR>() & theirBB;

        // Exclude promotion rank
        rightAttacks &= ~promRankBB;

        for (auto s: rightAttacks) {
            Square src = s - RIGHT_CAPT_DIR;
            Move move(src, s, SRC_PIECE, pos.getPieceAt(s), MT_SIMPLE_CAPTURE);
            ml.add(move);
        }
    }

    // Generate push-promotions
    if constexpr (GEN_SIMPLE_PROMOTIONS) {
        // Get unoccupied squares at the promotion rank bitboard
        Bitboard promRankBB = bbs::getRankBitboard(PROM_RANK) & ~occ;

        // Promoting pawns are one step behind the promotion rank
        Bitboard promotingPawns = promRankBB.shifted<-STEP_DIR>() & pawns;

        for (auto s: promotingPawns) {
            for (PieceType pt = PT_QUEEN; pt >= PT_KNIGHT; --pt) {
                Square dst = s + STEP_DIR;
                Move move(s, dst, SRC_PIECE, PIECE_NONE, MT_SIMPLE_PROMOTION, pt);
                ml.add(move);
            }
        }
    }

    // Generate en-passant captures
    if constexpr (GEN_EP) {
        Square epSquare = pos.getEnPassantSquare();
        if (epSquare != SQ_INVALID) {
            Bitboard epBB = BIT(epSquare);
            Bitboard epCapturers = epBB.shifted<-LEFT_CAPT_DIR>() | epBB.shifted<-RIGHT_CAPT_DIR>();
            epCapturers &= pawns;

            for (auto s: epCapturers) {
                Move move(s, epSquare, SRC_PIECE, PIECE_NONE, MT_EN_PASSANT_CAPTURE);
                ml.add(move);
            }
        }
    }

    // Generate pawn pushes
    if constexpr (GEN_SINGLE_PUSHES || GEN_DOUBLE_PUSHES) {
        // Pretend all squares in the promotion rank are occupied so that
        // we prevent "quiet" pushes to the promotion rank.
        Bitboard promRankBB = bbs::getRankBitboard(PROM_RANK);
        Bitboard pushOcc    = occ | promRankBB;

        // Single pushes (prevent pushing to occupied squares)
        Bitboard pushBB = pawns.shifted<STEP_DIR>() & ~pushOcc;

        if constexpr (GEN_SINGLE_PUSHES) {
            for (auto s: pushBB) {
                Square src = s - STEP_DIR;
                Move move(src, s, SRC_PIECE, PIECE_NONE, MT_NORMAL);
                ml.add(move);
            }
        }

        // Double pushes
        if constexpr (GEN_DOUBLE_PUSHES) {
            pushBB = pushBB.shifted<STEP_DIR>() & ~pushOcc;

            // Filter out pawns that can't perform double pushes
            pushBB &= bbs::getRankBitboard(DOUBLE_PUSH_RANK);

            Square doubleStep = STEP_DIR * 2;
            for (auto s: pushBB) {
                Square src = s - doubleStep;
                Move move(src, s, SRC_PIECE, PIECE_NONE, MT_DOUBLE_PUSH);
                ml.add(move);
            }
        }
    }
}

template<Color C, MoveTypeMask ALLOWED_FLAGS>
void generateKnightMoves(const Position &pos, MoveList &ml) {
    constexpr Piece SRC_PIECE = Piece(C, PT_KNIGHT);

    constexpr bool GEN_SIMPLE_CAPTURES = ALLOWED_FLAGS & BIT(MT_SIMPLE_CAPTURE);
    constexpr bool GEN_NORMAL = ALLOWED_FLAGS & BIT(MT_NORMAL);

    Bitboard knights = pos.getBitboard(Piece(C, PT_KNIGHT));
    Bitboard theirBB = pos.getBitboard(Piece(getOppositeColor(C), PT_NONE));
    Bitboard occ = pos.getCompositeBitboard();

    if constexpr (GEN_SIMPLE_CAPTURES) {
        // Capture generation -- only generate moves to squares populated by opponent pieces.
        for (auto ks: knights) {
            Bitboard attacks = bbs::getKnightAttacks(ks) & theirBB;
            for (auto s: attacks) {
                Move move(ks, s, SRC_PIECE, pos.getPieceAt(s), MT_SIMPLE_CAPTURE);
                ml.add(move);
            }
        }
    }

    if constexpr (GEN_NORMAL) {
        // Quiet move generation -- only generate moves to squares not populated
        // by pieces.
        for (auto ks: knights) {
            Bitboard attacks = bbs::getKnightAttacks(ks) & ~occ;
            for (auto s: attacks) {
                Move move(ks, s, SRC_PIECE, PIECE_NONE, MT_NORMAL);
                ml.add(move);
            }
        }
    }
}

template<Color C, MoveTypeMask ALLOWED_FLAGS>
void generateBishopMoves(const Position &pos, MoveList &ml) {
    constexpr Piece SRC_PIECE = Piece(C, PT_BISHOP);

    constexpr bool GEN_SIMPLE_CAPTURES = ALLOWED_FLAGS & BIT(MT_SIMPLE_CAPTURE);
    constexpr bool GEN_NORMAL = ALLOWED_FLAGS & BIT(MT_NORMAL);

    Bitboard bishops = pos.getBitboard(SRC_PIECE);
    Bitboard occ = pos.getCompositeBitboard();
    Bitboard theirBB = pos.getBitboard(Piece(getOppositeColor(C), PT_NONE));

    // Generate captures
    if constexpr (GEN_SIMPLE_CAPTURES) {
        for (auto src: bishops) {
            Bitboard attacks = bbs::getBishopAttacks(src, occ);

            // Only count attacks to squares occupied by opponent pieces
            attacks &= theirBB;

            for (auto dst: attacks) {
                Move move(src, dst, SRC_PIECE, pos.getPieceAt(dst), MT_SIMPLE_CAPTURE);
                ml.add(move);
            }
        }
    }

    // Generate quiet moves
    if constexpr (GEN_NORMAL) {
        for (auto src: bishops) {
            Bitboard attacks = bbs::getBishopAttacks(src, occ);

            // Only count attacks to non-occupied squares
            attacks &= ~occ;

            for (auto dst: attacks) {
                Move move(src, dst, SRC_PIECE, PIECE_NONE, MT_NORMAL);
                ml.add(move);
            }
        }
    }
}

template<Color C, MoveTypeMask ALLOWED_FLAGS>
void generateRookMoves(const Position &pos, MoveList &ml) {
    constexpr Piece SRC_PIECE = Piece(C, PT_ROOK);

    constexpr bool GEN_SIMPLE_CAPTURES = ALLOWED_FLAGS & BIT(MT_SIMPLE_CAPTURE);
    constexpr bool GEN_NORMAL = ALLOWED_FLAGS & BIT(MT_NORMAL);

    Bitboard rooks = pos.getBitboard(SRC_PIECE);
    Bitboard occ = pos.getCompositeBitboard();
    Bitboard theirBB = pos.getBitboard(Piece(getOppositeColor(C), PT_NONE));

    // Generate captures
    if constexpr (GEN_SIMPLE_CAPTURES) {
        for (auto src: rooks) {
            Bitboard attacks = bbs::getRookAttacks(src, occ);

            // Only count attacks to squares occupied by opponent pieces
            attacks &= theirBB;

            for (auto dst: attacks) {
                Move move(src, dst, SRC_PIECE, pos.getPieceAt(dst), MT_SIMPLE_CAPTURE);
                ml.add(move);
            }
        }
    }

    // Generate quiet moves
    if constexpr (GEN_NORMAL) {
        for (auto src: rooks) {
            Bitboard attacks = bbs::getRookAttacks(src, occ);

            // Only count attacks to non-occupied squares
            attacks &= ~occ;

            for (auto dst: attacks) {
                Move move(src, dst, SRC_PIECE, PIECE_NONE, MT_NORMAL);
                ml.add(move);
            }
        }
    }
}

template<Color C, MoveTypeMask ALLOWED_FLAGS>
void generateQueenMoves(const Position &pos, MoveList &ml) {
    constexpr Piece SRC_PIECE = Piece(C, PT_QUEEN);

    constexpr bool GEN_SIMPLE_CAPTURES = ALLOWED_FLAGS & BIT(MT_SIMPLE_CAPTURE);
    constexpr bool GEN_NORMAL = ALLOWED_FLAGS & BIT(MT_NORMAL);

    Bitboard queens = pos.getBitboard(SRC_PIECE);
    Bitboard occ = pos.getCompositeBitboard();
    Bitboard theirBB = pos.getBitboard(Piece(getOppositeColor(C), PT_NONE));

    // Generate captures
    if constexpr (GEN_SIMPLE_CAPTURES) {
        for (auto src: queens) {
            Bitboard attacks = bbs::getQueenAttacks(src, occ);

            // Only count attacks to squares occupied by opponent pieces
            attacks &= theirBB;

            for (auto dst: attacks) {
                Move move(src, dst, SRC_PIECE, pos.getPieceAt(dst), MT_SIMPLE_CAPTURE);
                ml.add(move);
            }
        }
    }

    // Generate quiet moves
    if constexpr (GEN_NORMAL) {
        for (auto src: queens) {
            Bitboard attacks = bbs::getQueenAttacks(src, occ);

            // Only count attacks to non-occupied squares
            attacks &= ~occ;

            for (auto dst: attacks) {
                Move move(src, dst, SRC_PIECE, PIECE_NONE, MT_NORMAL);
                ml.add(move);
            }
        }
    }
}

template<Color C, Side S, bool PSEUDO_LEGAL = false>
void generateCastles(const Position &pos, MoveList &ml) {
    constexpr Color THEM = getOppositeColor(C);

    constexpr Square SRC = C == CL_WHITE ? SQ_E1 : SQ_E8;

    constexpr Square DEST = S == SIDE_KING
                            ? (C == CL_WHITE ? SQ_G1 : SQ_G8)   // King-side
                            : (C == CL_WHITE ? SQ_C1 : SQ_C8);  // Queen-side

    constexpr Bitboard INNER_PATH = bbs::getInnerCastlePath(C, S);
    constexpr Bitboard KING_PATH = bbs::getKingCastlePath(C, S);

    constexpr Piece SRC_PIECE = Piece(C, PT_KING);

    if (pos.getPieceAt(SRC) != SRC_PIECE) {
        // No king at source square
        return;
    }

    if (!pos.getCastleRights(C, S)) {
        // Castling rights disallowed
        return;
    }

    Bitboard occ = pos.getCompositeBitboard();
    if (occ & INNER_PATH) {
        // There are pieces standing along the castling path
        return;
    }

    if (pos.getAttacks(THEM, PT_NONE) & KING_PATH) {
        // There are opposing pieces attacking the king path
        // Note that this already covers cases in which the king is in check,
        // since KING_PATH includes the king's square.
        return;
    }

    // Castles move can be generated, generate it.
    Move move(SRC, DEST, SRC_PIECE, PIECE_NONE, S == SIDE_KING ? MT_CASTLES_SHORT : MT_CASTLES_LONG);
    ml.add(move);
}

template<Color C, MoveTypeMask ALLOWED_FLAGS, bool PSEUDO_LEGAL = false>
void generateKingMoves(const Position &pos, MoveList &ml) {
    constexpr Piece SRC_PIECE = Piece(C, PT_KING);

    constexpr bool GEN_SIMPLE_CAPTURES = ALLOWED_FLAGS & BIT(MT_SIMPLE_CAPTURE);
    constexpr bool GEN_NORMAL = ALLOWED_FLAGS & BIT(MT_NORMAL);

    Bitboard kingBB = pos.getBitboard(Piece(C, PT_KING));
    Bitboard theirBB = pos.getBitboard(Piece(getOppositeColor(C), PT_NONE));
    Bitboard occ = pos.getCompositeBitboard();

    if constexpr (GEN_SIMPLE_CAPTURES) {
        // Capture generation -- only generate moves to squares populated by opponent pieces.
        for (auto ks: kingBB) {
            Bitboard attacks = bbs::getKingAttacks(ks) & theirBB;
            for (auto s: attacks) {
                Move move(ks, s, SRC_PIECE, pos.getPieceAt(s), MT_SIMPLE_CAPTURE);
                ml.add(move);
            }
        }
    }

    if constexpr (GEN_NORMAL) {
        // Quiet move generation -- only generate moves to squares not populated
        // by pieces.
        for (auto ks: kingBB) {
            Bitboard attacks = bbs::getKingAttacks(ks) & ~occ;
            for (auto s: attacks) {
                Move move(ks, s, SRC_PIECE, PIECE_NONE, MT_NORMAL);
                ml.add(move);
            }
        }
    }

    // Generate castling moves
    if constexpr (ALLOWED_FLAGS & BIT(MT_CASTLES_SHORT)) {
        generateCastles<C, SIDE_KING, PSEUDO_LEGAL>(pos, ml);
    }
    if constexpr (ALLOWED_FLAGS & BIT(MT_CASTLES_LONG)) {
        generateCastles<C, SIDE_QUEEN, PSEUDO_LEGAL>(pos, ml);
    }
}

template <Color C, MoveTypeMask ALLOWED_MOVE_TYPES, PieceTypeMask ALLOWED_PIECE_TYPES, bool PSEUDO_LEGAL = false>
void generateAll(const Position& pos, MoveList& ml) {
    constexpr bool GEN_PAWN   = (ALLOWED_PIECE_TYPES & BIT(PT_PAWN)) != 0;
    constexpr bool GEN_KNIGHT = (ALLOWED_PIECE_TYPES & BIT(PT_KNIGHT)) != 0;
    constexpr bool GEN_BISHOP = (ALLOWED_PIECE_TYPES & BIT(PT_BISHOP)) != 0;
    constexpr bool GEN_ROOK   = (ALLOWED_PIECE_TYPES & BIT(PT_ROOK)) != 0;
    constexpr bool GEN_QUEEN  = (ALLOWED_PIECE_TYPES & BIT(PT_QUEEN)) != 0;
    constexpr bool GEN_KING   = (ALLOWED_PIECE_TYPES & BIT(PT_KING)) != 0;

    if constexpr (GEN_PAWN)
        utils::generatePawnMoves<C, ALLOWED_MOVE_TYPES>(pos, ml);

    if constexpr (GEN_KNIGHT)
        utils::generateKnightMoves<C, ALLOWED_MOVE_TYPES>(pos, ml);

    if constexpr (GEN_BISHOP)
        utils::generateBishopMoves<C, ALLOWED_MOVE_TYPES>(pos, ml);

    if constexpr (GEN_ROOK)
        utils::generateRookMoves<C, ALLOWED_MOVE_TYPES>(pos, ml);

    if constexpr (GEN_QUEEN)
        utils::generateQueenMoves<C, ALLOWED_MOVE_TYPES>(pos, ml);

    if constexpr (GEN_KING)
        utils::generateKingMoves<C, ALLOWED_MOVE_TYPES, PSEUDO_LEGAL>(pos, ml);
}

} // utils

/**
 * Generates all legal moves in a given position.
 *
 * Legal moves take into consideration the fact that the current color to move
 * cannot have any pieces attacking the opposing king (legality).
 *
 * Use move type masks to generate only specific
 * types of moves if desired.
 *
 * @tparam ALLOWED_MOVE_TYPES Mask containing which types of moves should be generated.
 * @param ml The move list to append generated moves to.
 * @return The number of generated moves.
 */
template<MoveTypeMask ALLOWED_MOVE_TYPES = MTM_ALL, PieceTypeMask ALLOWED_PIECE_TYPES = PTM_ALL, bool PSEUDO_LEGAL = false>
int generate(const Position &pos, MoveList &ml) {
    MoveList moves;
    int initialCount = ml.size();
    if (pos.getColorToMove() == CL_WHITE) {
        // Generate moves with white pieces
        utils::generateAll<CL_WHITE,
                           ALLOWED_MOVE_TYPES,
                           ALLOWED_PIECE_TYPES,
                           PSEUDO_LEGAL>(pos, moves);
    } else {
        // Generate moves with black pieces
        utils::generateAll<CL_BLACK,
                ALLOWED_MOVE_TYPES,
                ALLOWED_PIECE_TYPES,
                PSEUDO_LEGAL>(pos, moves);

    for (auto move: moves) {
        if (PSEUDO_LEGAL || pos.isMoveLegal(move)) {
            ml.add(move);
        }
    }

    return ml.size() - initialCount;
}

} // movegen

} // lunachess

#endif // LUNA_MOVEGEN_H