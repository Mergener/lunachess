#ifndef LUNA_POSITION_H
#define LUNA_POSITION_H

#include <string>
#include <vector>
#include <array>
#include <optional>
#include <string_view>
#include <sstream>

#include "debug.h"
#include "zobrist.h"
#include "move.h"
#include "types.h"
#include "bitboard.h"
#include "staticlist.h"

namespace lunachess {

enum ChessResult {
    RES_UNFINISHED,
    RES_DRAW_STALEMATE,
    RES_DRAW_REPETITION,
    RES_DRAW_TIME_NOMAT,
    RES_DRAW_NOMAT,
    RES_DRAW_RULE50,
    RES_WIN_CHECKMATE,
    RES_WIN_TIME,
    RES_WIN_RESIGN,
    RES_LOSS_CHECKMATE,
    RES_LOSS_TIME,
    RES_LOSS_RESIGN,

    //
    // Constants below create closed intervals for wins/draws/loss.
    // Use them to check for simple results.
    // Ex.:
    // bool isWin(const Position& pos, Color c, i64 remainingTime) {
    //     ChessResult res = pos.getResult(c, remainingTime > 0);
    //     return res >= RES_WIN_BEGIN && res <= RES_WIN_END;
    // }
    //

    RES_DRAW_BEGIN = RES_DRAW_STALEMATE,
    RES_DRAW_END = RES_DRAW_RULE50,
    RES_WIN_BEGIN = RES_WIN_CHECKMATE,
    RES_WIN_END = RES_WIN_RESIGN,
    RES_LOSS_BEGIN = RES_LOSS_CHECKMATE,
    RES_LOSS_END = RES_LOSS_RESIGN
};

inline constexpr bool isWin(ChessResult r) {
    return r >= RES_WIN_BEGIN && r <= RES_WIN_END;
}

inline constexpr bool isLoss(ChessResult r) {
    return r >= RES_LOSS_BEGIN && r <= RES_LOSS_END;
}

inline constexpr bool isDraw(ChessResult r) {
    return r >= RES_DRAW_BEGIN && r <= RES_DRAW_END;
}

class Position {
public:
    //
    // Position base data
    //

    inline Piece getPieceAt(Square s) const { return m_Pieces[s]; }
    inline void setPieceAt(Square s, Piece p) {
        setPieceAt<true, true>(s, p);
    }

    inline Color getColorToMove() const { return m_ColorToMove; }
    void setColorToMove(Color c);

    inline Square getKingSquare(Color c) const {
        Bitboard kingBB = getBitboard(Piece(c, PT_KING));
        if (kingBB == 0) {
            return SQ_INVALID;
        }
        return *kingBB.begin();
    }
    inline bool isSquareAttacked(Square s, Color attacker) const {
        return m_Status.attacks[PT_NONE][attacker].contains(s);
    }

    inline CastlingRightsMask getCastleRights() const { return m_Status.castleRights; }
    void setCastleRights(CastlingRightsMask crm);

    inline constexpr bool getCastleRights(Color color, Side side) const {
        const ui8 bit = BIT((static_cast<ui8>(color) * 2) + static_cast<ui8>(side));

        return (m_Status.castleRights & bit) != CR_NONE;
    }
    void setCastleRights(Color color, Side side, bool allow);

    inline Square getEnPassantSquare() const { return m_Status.epSquare; }
    void setEnPassantSquare(Square s);

    inline ui64 getZobrist() const { return m_Status.zobrist; }

    inline int getPlyCount() { return m_PlyCount; }

    inline Bitboard getCompositeBitboard() const { return m_Composite; }
    inline Bitboard getBitboard(Piece p) const { return m_BBs[p.getType()][p.getColor()]; }

    inline Bitboard getAttacks(Color c, PieceType pt = PT_NONE) const {
        return m_Status.attacks[pt][c];
    }

    inline Bitboard getPinned() const { return m_Pinned; }
    inline Square getPinner(Square s) const { return m_Pinner[s]; }

    //
    // Position status
    //

    inline bool is50MoveRuleDraw() const { return m_Status.fiftyMoveCounter >= 100; }

    inline bool isDraw(int maxRepetitions = 2) const {
        return is50MoveRuleDraw() || isRepetitionDraw(maxRepetitions) || isInsufficientMaterialDraw();
    }

    bool colorHasSufficientMaterial(Color c) const;

    inline bool isInsufficientMaterialDraw() const {
        return !colorHasSufficientMaterial(CL_WHITE) &&
               !colorHasSufficientMaterial(CL_BLACK);
    }

    /**
     *  Returns true if the position is a draw by repetition.
     */
    bool isRepetitionDraw(int maxRepetitions = 3) const;

    /**
     * Returns true if the position is legal. A position is legal
     * if the current color to move doesn't have any pieces attacking the opposing
     * king.
     */
    bool legal() const;

    inline bool isMoveLegal(Move move) const {
        if (isCheck()) {
            return isMoveLegal<true>(move);
        }
        return isMoveLegal<false>(move);
    }

    /**
     * Returns true if the position is in a check. A check happens
     * when the king of the current color to move is under attack.
     */
    inline bool isCheck() const {
        return m_Status.nCheckers > 0;
    }

    /**
     * Computes the sum of the point values of all remaining pieces on the board.
     * @tparam EXCLUDE_PAWNS If true, excludes pawns from the count.
     * @return The material count.
     */
    template <bool EXCLUDE_PAWNS = false, Color C = CL_COUNT>
    int countMaterial() const {
        PieceType initialPt;

        if (EXCLUDE_PAWNS) {
            initialPt = PT_PAWN + 1;
        }
        else {
            initialPt = PT_PAWN;
        }

        int total = 0;

        for (PieceType pt = initialPt; pt < PT_KING; ++pt) {
            if constexpr (C != CL_BLACK) {
                Bitboard wbb = getBitboard(Piece(CL_WHITE, pt));
                total += wbb.count() * getPiecePointValue(pt);
            }
            if constexpr (C != CL_WHITE) {
                Bitboard bbb = getBitboard(Piece(CL_BLACK, pt));
                total += bbb.count() * getPiecePointValue(pt);
            }
        }

        return total;
    }

    template <bool EXCLUDE_PAWNS = false>
    int countMaterial(Color c) const {
        if (c == CL_WHITE) {
            return countMaterial<EXCLUDE_PAWNS, CL_WHITE>();
        }
        return countMaterial<EXCLUDE_PAWNS, CL_BLACK>();
    }

    inline bool isPinned(Square s) const {
        return m_Pinned.contains(s);
    }

    //
    // Move-related methods
    //

    /**
     * Makes a move on the board. Can later be undone using undoMove().
     *
     * Does not validate the move, thus this method accepts illegal/invalid
     * moves as well.
     *
     * Invalid moves can lead the position to unpredictable states.
     * Only make moves generated by a generate() method from this position or
     * make sure they're validated using the move.isValid() method.
     *
     * @param move The move to be played.
     */
    void makeMove(Move move);

    /** Undoes the effects of the last move made on the board.
        Assumes a move has been previously made. */
    void undoMove();

    /**
     * Makes a null move on the board.
     * A null move is a move that doesn't affect the placement of any pieces.
     * It could be considered as a 'pass my move' move.
     */
    void makeNullMove();

    /**
     * Undoes the effects of a null move. Assumes a null move has been previously made
     * and it was the last move to be made.
     */
    void undoNullMove();

    //
    // Position construction/destruction/serialization/deserialization
    //

    /**
     * Returns a FEN string that represents this position.
     */
    std::string toFen() const;

    ChessResult getResult(Color pov, bool colorToMoveHasTime = true) const;

    Position();
    Position(const Position& rhs) = default;
    Position(Position&& rhs) = default;
    Position& operator=(const Position& rhs) = default;
    ~Position() = default;

    static Position getInitialPosition();
    static std::optional<Position> fromFen(std::string_view fen);

private:
    struct Status {
        Move lastMove = MOVE_INVALID;
        ui64 zobrist = 5454; // Any number
        int fiftyMoveCounter = 0;

        /** En-passant capture square */
        Square epSquare = SQ_INVALID;

        CastlingRightsMask castleRights = CR_NONE;

        Bitboard attacks[PT_COUNT][CL_COUNT];
        int nCheckers = 0;
    };
    Status m_Status;

    /** Previous statuses stack. */
    std::vector<Status> m_PrevStatuses;

    /** Mailbox representation of the board. */
    std::array<Piece, 64> m_Pieces;

    Color m_ColorToMove = CL_WHITE;

    Bitboard m_BBs[PT_COUNT][CL_COUNT];
    Bitboard m_Composite = 0;

    /** Bitboard of all pinned pieces */
    Bitboard m_Pinned;
    std::array<Square, 64> m_Pinner;

    //
    // Private methods
    //

    void updateAttacks();
    void updatePins();
    void scanPins(Bitboard attackers, Square kingSquare, Color pinnedColor);

    template <bool DO_ZOBRIST, bool DO_PINS_ATKS>
    void setPieceAt(Square s, Piece p);

    int m_PlyCount = 0;

    void handleSpecialMove(Move move);
    void handleSpecialMoveUndo();
    void handleCastleUndo(Side side);

    Square getSmallestAttackerSquare(Square s, Color c) const;

    template <bool CHECK>
    bool isMoveLegal(Move move) const;

    void refreshCastles();
};

std::ostream& operator<<(std::ostream& stream, const Position& pos);

template <bool DO_ZOBRIST, bool DO_PINS_ATKS>
void Position::setPieceAt(Square s, Piece p) {
    ASSERT_VALID_SQUARE(s);

    Piece prev = getPieceAt(s);

    if (prev != PIECE_NONE) {
        // Remove from previous bbs
        m_BBs[prev.getType()][prev.getColor()].remove(s);
        m_BBs[PT_NONE][prev.getColor()].remove(s);

        if constexpr (DO_ZOBRIST) {
            m_Status.zobrist ^= zobrist::getPieceSquareKey(prev, s);
        }
    }

    m_Pieces[s] = p;

    if (p != PIECE_NONE) {
        m_Composite.add(s);

        m_BBs[p.getType()][p.getColor()].add(s);
        m_BBs[PT_NONE][p.getColor()].add(s);

        if constexpr (DO_ZOBRIST) {
            m_Status.zobrist ^= zobrist::getPieceSquareKey(p, s);
        }
    }
    else {
        m_BBs[p.getType()][p.getColor()].remove(s);
        m_BBs[PT_NONE][p.getColor()].remove(s);

        m_Composite.remove(s);
    }

    if constexpr (DO_PINS_ATKS) {
        updateAttacks();
        updatePins();
        refreshCastles();
    }
}

//
//  Move generation
//

template <bool CHECK>
bool Position::isMoveLegal(Move move) const {
    Color us = getColorToMove();
    Square ourKing = getKingSquare(us);
    if (ourKing == SQ_INVALID) {
        return true;
    }

    Color them = getOppositeColor(us);
    Bitboard occ = getCompositeBitboard();
    Square src = move.getSource();
    Square dest = move.getDest();
    Piece srcPiece = move.getSourcePiece();

    // Regardless of the position being a check or not, pinned
    // pieces can only move alongside their pins.
    if (isPinned(src)) {
        Square pinner = m_Pinner[src];
        Bitboard between = bbs::getSquaresBetween(ourKing, pinner);
        between.add(pinner);

        if (!between.contains(dest)) {
            // Trying to move away from pin, illegal.
            return false;
        }
    }

    // En-passants must be treated with extra care.
    if (move.getType() == MT_EN_PASSANT_CAPTURE) {
        Square capturedPawnSq = dest + getPawnStepDir(them);
        Bitboard epOcc = occ;

        // Pretend there are no pawns
        epOcc.remove(capturedPawnSq);
        epOcc.remove(src);

        Bitboard kingRankBB = bbs::getRankBitboard(getRank(ourKing));

        Bitboard theirRooks = getBitboard(Piece(them, PT_ROOK));
        Bitboard theirQueens = getBitboard(Piece(them, PT_QUEEN));
        Bitboard theirHorizontalAtks = bbs::getRookAttacks(ourKing, epOcc) & (theirRooks | theirQueens) & kingRankBB;

        if (bbs::getRookAttacks(ourKing, epOcc) & theirHorizontalAtks) {
            return false;
        }
    }
    else if (srcPiece.getType() == PT_KING) {
        // King cannot be moving to a square attacked by any piece
        if (m_Status.attacks[PT_NONE][them].contains(dest)) {
            return false;
        }

        // The conditional above does not cover cases where the king runs
        // in the same direction a slider is attacking them.
        Bitboard occWithoutKing = occ & (~BIT(ourKing));

        Bitboard theirBishops = getBitboard(Piece(them, PT_BISHOP));
        Bitboard theirRooks = getBitboard(Piece(them, PT_ROOK));
        Bitboard theirQueens = getBitboard(Piece(them, PT_QUEEN));

        Bitboard theirDiagonalAtks = bbs::getBishopAttacks(dest, occWithoutKing) & (theirBishops | theirQueens);
        if (theirDiagonalAtks != 0) {
            return false;
        }

        Bitboard theirLineAtks = bbs::getRookAttacks(dest, occWithoutKing) & (theirRooks | theirQueens);
        if (theirLineAtks != 0) {
            return false;
        }
    }
    else if constexpr (CHECK) {
        if (m_Status.nCheckers > 1) {
            // Only king moves allowed in double checks
            return false;
        }

        // We're in a single check and trying to move a piece that is not the king.
        // The piece we're trying to move can only move to a square between the king
        // and the checker...
        Square atkSquare = getSmallestAttackerSquare(ourKing, them);
        Bitboard between = bbs::getSquaresBetween(ourKing, atkSquare);
        between.add(atkSquare); // ... or capture the checker!

        if (!between.contains(dest)) {
            return false;
        }
    }
    return true;
}

}

#endif // LUNA_POSITION_H