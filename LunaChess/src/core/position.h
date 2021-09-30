#ifndef LUNA_POSITION_H
#define LUNA_POSITION_H

#include <cstring>
#include <string>
#include <string_view>
#include <optional>

#include "defs.h"
#include "types.h"
#include "move.h"
#include "staticlist.h"
#include "piece.h"
#include "bitboard.h"
#include "zobrist.h"

namespace lunachess {

struct PerftStats {
	ui64 leafNodes = 0;
	ui64 captures = 0;
	ui64 enPassants = 0;
	ui64 promotions = 0;
	ui64 castles = 0;
};

class Position {
public:
	using DrawList = StaticList<ui64, 96>;

	ui64 perft(int depth) const;

	void richPerft(int depth, PerftStats& stats) const; 

	bool sideHasSufficientMaterial(Side side) const;

    int getMaterialCount(Side side) const;

	inline bool isInsufficientMaterialDraw() {
		return !sideHasSufficientMaterial(Side::White) &&
			!sideHasSufficientMaterial(Side::Black);
	}

	/**
		Returns this position's bitboard for a given piece type in a given side.
		Undefined behavior if side == Side::None or pt == PieceType::None.
	*/
	inline const Bitboard& getPieceBitboard(PieceType pt, Side side) const {
		return const_cast<Position *>(this)->getPieceBitboardInternal(pt, side);
	}

	/**
		Returns this position's bitboard for a given piece type in a given side.
		Undefined behavior if side == Side::None or pt == PieceType::None.
	*/
	inline const Bitboard& getPieceBitboard(const Piece& piece) const {
		return getPieceBitboard(piece.getType(), piece.getSide());
	}

	/**
		Returns the bitboard that indicates all occupied squares in this position. (aka. occupancy bitboard)
	*/
	inline const Bitboard& getCompositeBitboard() const {
		return m_CompositeBitboard;
	}

	inline bool pieceXrays(Square pieceSquare, Square xraySquare) {
        LUNA_ASSERT(squares::isValid(pieceSquare), "Must be a valid square. (got " << (int)pieceSquare << ")");
        LUNA_ASSERT(squares::isValid(xraySquare), "Must be a valid square. (got " << (int)xraySquare << ")");

		auto piece = getPieceAt(pieceSquare);
		return bitboards::getPieceAttacks(piece.getType(), 0, pieceSquare, piece.getSide()).contains(xraySquare);
	}

	/**
		Returns whether a player can castle in a given lateral side (king-side or queen-side).
	*/
	bool getCastleRights(Side side, LateralSide lSide) const;

	inline CastlingRightsMask getCastleRightsMask() const { return m_CastleMask; }

	/**
		Changes a player's rights to castle in a given lateral side (king-side or queen-side).
	*/
	void setCastleRights(Side side, LateralSide lSide, bool val);

	void setCastleRights(CastlingRightsMask crm);

	inline ui64 getZobristKey() const { return m_Zobrist; }

	/**
		Returns the capturable en-passant square in this position.
		If there are none, SQ_INVALID is returned.
	*/
	inline Square getEnPassantSquare() const { return m_EnPassantSquare; }

	/**
		Changes this position's current en passant square.
	*/
	inline void setEnPassantSquare(Square square) {
        LUNA_ASSERT(square == SQ_INVALID || squares::rankOf(square) == 2 || squares::rankOf(square) == 5,
                    "Invalid square for en passant (got " << (int)square << ")");

		if (m_EnPassantSquare != SQ_INVALID) {
			m_Zobrist ^= zobrist::getEnPassantSquareKey(m_EnPassantSquare);
		}

		m_EnPassantSquare = square;

		if (m_EnPassantSquare != SQ_INVALID) {
			m_Zobrist ^= zobrist::getEnPassantSquareKey(m_EnPassantSquare);
		}
	}

	/**
		Returns the piece located at the specified square.
	*/
	inline Piece getPieceAt(Square sqr) const {
        LUNA_ASSERT(squares::isValid(sqr), "Must be a valid square. (got " << (int)sqr << ")");

		return m_Squares[sqr];
	}

	/**
		Sets the piece located on the specified square.
	*/
	void setPieceAt(Square sqr, Piece piece);

	/**
		Fills the given move list with all pseudo legal moves.
		Returns the number of moves added to the list.
	*/
	int getPseudoLegalMoves(MoveList& l, ui64 moveFlagsMask = MoveFlags::All) const;

	/**	Retrieves all legal moves to a specified buffer.
		Legal moves are pseudo legal moves in which moving pieces do not
		leave their own king in check.
		Expects the buffer to be of size 256 bytes or more.
	*/
	int getLegalMoves(MoveList& l, ui64 moveFlagsMask = MoveFlags::All) const;

	/**
		Makes a move on the board. If validate is set to false, the move 
		will be made regardless of being legal or not, and this method will always
		return true. 
		If validate is set to true, the move will only be applied if it is considered legal,
		and this method will only return true if the move gets applied.
	*/
	bool makeMove(Move move, bool validate = false, bool fullyReversibleDrawList = false);

	/**
		Reverts a given move, restoring the state of the position prior to when
		the move was made.
	*/
	void undoMove(Move move);

	/**
		Returns true if the current state of this position is legal.
		A position is considered legal if either no kings are in check or
		a single king is in check, and that king must be of the same side
		as the current side to move.
	*/
	bool legal() const;

	inline int getPlyCounter() const { return m_Ply; }

	/**
		Returns the number of moves counted for 50-move-rule draws.
		If this number reaches 50, this position is considered a draw
		by the fifty move rule.

		Important note: 1 move == 2 plies.
	*/
	inline int get50moveRuleCounter() const { return m_50MoveClock / 2; }

	/**
		True if this position is considered a draw by the fifty move rule.
	*/
	inline bool is50moveRuleDraw() const { return m_50MoveClock >= 100; }

	Square getKingSquare(Side side) const;

	/**
		True if the current side to move's king is currently under attack.
	*/
	bool isCheck() const;

	/**
		True if a position equal to this one has appeared twice prior to this.
	*/
	bool isRepetitionDraw(int maxRepetitions = 3) const;

	inline bool isDraw(int maxRepetitions = 3) {
		return is50moveRuleDraw() ||
			   isInsufficientMaterialDraw() ||
			   isRepetitionDraw(maxRepetitions);
	}

	/**
		Returns the current side to move.
	*/
	inline Side getSideToMove() const { return m_SideToMove; }

	/**
		Changes the current side to move.
	*/
	inline void setSideToMove(Side side) {
		LUNA_ASSERT(side != Side::None, "Cannot set sideToMove to Side::None.");

		m_Zobrist ^= zobrist::getSideToMoveKey(m_SideToMove);
		m_SideToMove = side;
		m_Zobrist ^= zobrist::getSideToMoveKey(m_SideToMove);
	}

	/**
		Returns true if the specified move is pseudo-legal in this position.
		Pseudo-legal moves do not take in consideration whether the position
		that arises from them is illegal or not.
	*/
	bool moveIsPseudoLegal(Move move) const;

	/**
		Returns true if the specified move is legal in this position.
	*/
	bool moveIsLegal(Move move) const;

	static Position getInitialPosition(bool castlingAllowed = true);

	inline Position() {
		std::memset(m_BBs, 0, sizeof(m_BBs));
		std::memset(m_Squares, 0, sizeof(m_Squares));
	}

	int countTotalPointValue(bool includePawns = true) const;

	bool canCastleNow(LateralSide lateralSide) const;

	std::string toFen() const;

	static std::optional<Position> fromFEN(std::string_view fen);

	Position(Position&& other) noexcept = default;

	inline Position(const Position& other) {
        std::memcpy(this, &other, sizeof(Position));
    }

	inline Position& operator=(const Position& other) {
		std::memcpy(this, &other, sizeof(Position));
		return *this;
	}

	/**
		Returns this position's draw list. A draw list is a list
		of the zobrist key of positions prior to this one. The list
		is used to determine whether a position is a draw by threefold repetition.
	*/
	inline const DrawList& getDrawList() const { return m_DrawList; }

private:
	void handleSpecialMove(Move move);

	void handleSpecialMoveUndo(Move move);
	void castleUndoHandler(LateralSide castlingSide);

	ui64 perftInternal(int depth);

	void richPerftInternal(int depth, PerftStats& stats);

	inline Bitboard& getPieceBitboardInternal(const Piece& piece) {
		return getPieceBitboardInternal(piece.getType(), piece.getSide());
	}

	inline Bitboard& getPieceBitboardInternal(PieceType pt, Side side) {
		return m_BBs[(int)side][(int)pt];
	}

	inline Bitboard& getSideBitboard(Side side) {
		return m_BBs[(int)side][(int)PieceType::None];
	}

	/** Each bitboard from this array contains every placements for a given piece type and side. */
	Bitboard m_BBs[3][(int)PieceType::_Count];

	/** Bitboard that holds the information of all squares being occupied by a piece. */
	Bitboard m_CompositeBitboard = 0;
	
	Piece m_Squares[64];

	Side m_SideToMove = Side::White;

	int m_Ply = 0;

	int m_50MoveClock = 0;

	ui64 m_Zobrist = 0;

	Square m_EnPassantSquare = SQ_INVALID;

	CastlingRightsMask m_CastleMask = CastlingRightsMask::All;

	DrawList m_DrawList;
};

std::ostream& operator<<(std::ostream& stream, const Position& pos);

}

#endif // LUNA_POSITION_H
