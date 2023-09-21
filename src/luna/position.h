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

    inline int getPlyCount() const { return m_PlyCount; }

    /**
     * Returns a bitboard of all occupied squares.
     */
    inline Bitboard getCompositeBitboard() const { return m_Composite; }

    /**
     * Returns a bitboard with the location of all pieces equal to the one
     * specified. If the specified piece's type is of type PT_NONE, returns
     * all the pieces of the specified piece's color.
     * Ex: Specifying p as Piece(CL_BLACK, PT_NONE) will return a bitboard
     * with all black pieces' squares.
     */
    inline Bitboard getBitboard(Piece p) const { return m_BBs[p.getType()][p.getColor()]; }

    inline Bitboard getAttacks(Color c, PieceType pt = PT_NONE) const {
        return m_Status.attacks[pt][c];
    }

    /**
     * Returns a bitboard with the squares of all pieces that are
     * pinned to their respective king.
     */
    inline Bitboard getPinned() const { return m_Pinned; }

    /**
     * For a given square s, returns the square of a piece that is currently
     * pinning a piece on square s to its king.
     * Returns SQ_INVALID if there is no pinner.
     */
    inline Square getPinner(Square s) const { return m_Pinner[s]; }

    //
    // Position status
    //

    inline Move getLastMove() const { return m_Status.lastMove; }

    inline i32 get50MoveRulePlyCounter() const { return m_Status.fiftyMoveCounter; }

    inline bool is50MoveRuleDraw() const { return m_Status.fiftyMoveCounter >= 100; }

    inline bool isDraw(int maxPositionAppearances = 3) const {
        return is50MoveRuleDraw() || isRepetitionDraw(maxPositionAppearances) || isInsufficientMaterialDraw();
    }

    bool colorHasSufficientMaterial(Color c) const;

    inline bool isInsufficientMaterialDraw() const {
        return !colorHasSufficientMaterial(CL_WHITE) &&
               !colorHasSufficientMaterial(CL_BLACK);
    }

    /**
     *  Returns true if the position is a draw by repetition.
     */
    bool isRepetitionDraw(int maxAppearances = 3) const;

    /**
     * Returns true if the position is legal. A position is legal
     * if the current color to move doesn't have any pieces attacking the opposing
     * king.
     */
    inline bool legal() const {
        Color us = getColorToMove();
        Color them = getOppositeColor(us);
        Square kingSquare = getKingSquare(them);
        return !isSquareAttacked(kingSquare, us);
    }

    bool isMovePseudoLegal(Move move) const;

    inline bool isMoveLegal(Move move) const {
        if (isCheck()) {
            return isMoveLegal<true>(move);
        }
        return isMoveLegal<false>(move);
    }

    inline bool givesCheck(Move move) const {
        // Normal check
        Piece p      = move.getSourcePiece();
        Color c      = p.getColor();
        Square ks    = getKingSquare(getOppositeColor(c));
        Bitboard occ = getCompositeBitboard();
        Bitboard atksFromDest = bbs::getPieceAttacks(move.getDest(), occ, p);

        if (atksFromDest.contains(ks)) {
            return true;
        }

        // Discovered check -- remove the piece from the occupancy and see
        // if a slider would attack the king
        occ.remove(move.getSource());
        Bitboard queens = getBitboard(Piece(c, PT_QUEEN));
        Bitboard vertSliders = queens | getBitboard(Piece(c, PT_ROOK));
        Bitboard diagSliders = queens | getBitboard(Piece(c, PT_BISHOP));
        if ((bbs::getBishopAttacks(ks, occ) & diagSliders) != 0) {
            // Discovered diagonal attack
            return true;
        }
        if ((bbs::getRookAttacks(ks, occ) & vertSliders) != 0) {
            // Discovered diagonal attack
            return true;
        }

        return false;
    }

    /**
     * Returns true if the position is in a check. A check happens
     * when the king of the current color to move is under attack.
     */
    inline bool isCheck() const {
        return m_Status.nCheckers > 0;
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

    /**
     * Undoes the effects of the last move made on the board.
     * Assumes a move has been previously made.
     */
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
        ui64 zobrist = 5454;
        int fiftyMoveCounter = 0;
        CastlingRightsMask castleRights = CR_NONE;
        Bitboard attacks[PT_COUNT][CL_COUNT];
        int nCheckers = 0;
        Square epSquare = SQ_INVALID;
    };
    Status m_Status;

    std::vector<Status> m_PrevStatuses;
    std::array<Piece, 64> m_Pieces;
    int m_PlyCount = 0;
    Bitboard m_BBs[PT_COUNT][CL_COUNT];
    Bitboard m_Composite = 0;

    Color m_ColorToMove = CL_WHITE;

    /** Bitboard of all pinned pieces */
    Bitboard m_Pinned;
    std::array<Square, 64> m_Pinner;


    void updateAttacks();
    void updatePins();
    void scanPins(Bitboard attackers, Square kingSquare, Color pinnedColor);

    template <bool DO_ZOBRIST, bool DO_PINS_ATKS>
    void setPieceAt(Square s, Piece p);

    void handleSpecialMove(Move move);
    void handleSpecialMoveUndo();
    void handleCastleUndo(Side side);

    Square getSmallestAttackerSquare(Square s, Color c) const;

    template <bool CHECK>
    bool isMoveLegal(Move move) const;

    bool isMoveMovementValid(Move move) const;
    bool isCastlesPseudoLegal(Square kingSquare, Color c, Side castlingSide) const;

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
    Color us       = getColorToMove();
    Square ourKing = getKingSquare(us);
    if (ourKing == SQ_INVALID) {
        // Any pseudo legal move is legal with no king
        // on the board.
        return true;
    }

    Color them     = getOppositeColor(us);
    Bitboard occ   = getCompositeBitboard();
    Square src     = move.getSource();
    Square dest    = move.getDest();
    Piece srcPiece = move.getSourcePiece();

    // Regardless of the position being a check or not, pinned
    // pieces can only move alongside their pins.
    if (isPinned(src)) {
        Square pinner    = m_Pinner[src];
        Bitboard between = bbs::getSquaresBetween(ourKing, pinner);
        between.add(pinner);

        if (!between.contains(dest)) {
            // Trying to move away from pin, illegal.
            return false;
        }
    }

    // En-passant must be treated with extra care.
    if (move.getType() == MT_EN_PASSANT_CAPTURE) {
        Square capturedPawnSq = dest + getPawnStepDir(them);
        Bitboard epOcc = occ;

        // Pretend there are no pawns
        epOcc.remove(capturedPawnSq);
        epOcc.remove(src);

        Bitboard kingRankBB = bbs::getRankBitboard(getRank(ourKing));

        Bitboard theirRooks  = getBitboard(Piece(them, PT_ROOK));
        Bitboard theirQueens = getBitboard(Piece(them, PT_QUEEN));
        Bitboard theirHorizontalAtks = bbs::getRookAttacks(ourKing, epOcc) & (theirRooks | theirQueens) & kingRankBB;

        if (bbs::getRookAttacks(ourKing, epOcc) & theirHorizontalAtks) {
            return false;
        }

        if (CHECK) {
            Bitboard theirBishops = getBitboard(Piece(them, PT_BISHOP));
            Bitboard theirDiagAttackers = theirBishops | theirQueens;
            if (bbs::getBishopAttacks(ourKing, occ) & theirDiagAttackers) {
                return false;
            }
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

} // lunachess

#endif // LUNA_POSITION_H
