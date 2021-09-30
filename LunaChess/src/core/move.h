#ifndef LUNA_MOVE_H
#define LUNA_MOVE_H

#include <iostream>
#include <utility>

#include "defs.h"
#include "piece.h"
#include "square.h"
#include "bitboard.h"
#include "debug.h"

namespace lunachess {

class Position;

class Move {
	/*
		Encoding: 
			bits 0-5: source square
			bits 6-11: dest square
			bits 12-13: source piece side
			bits 14-16: source piece type
			bits 17-18: destination piece side
			bits 19-21: destination piece type
			bits 22-28: previous en-passant square
			bits 29-31: desired promotion piece type
			bits 32-35: previous castling rights
				bit 32: white king-side castling right
				bit 33: white queen-side castling right
				bit 34: black king-side castling right
				bit 35: black queen-side castling right
	*/

public:
	inline static Move getInvalidMove() {
		Move move;
		move.m_Move = 0;
		return move;
	}

	inline ui64 getEncodedMove() const { return m_Move; }

	inline Square getSource() const { return m_Move & 0x3f; }
	inline Square getDest() const { return (m_Move >> 6) & 0x3f; }

	/** Castling rights mask before this move was played. */
	inline CastlingRightsMask getPreviousCastleRights() const { 
		return static_cast<CastlingRightsMask>((m_Move >> 32) & 0xf); 
	}

	/** Piece being moved. */
	inline Piece getSourcePiece() const {
		return Piece(static_cast<Side>((m_Move >> 12) & 0x3), 
			         static_cast<PieceType>((m_Move >> 14) & 0x7)); 
	}

	/** Piece at the destination square. Can be PIECE_NONE. For captures, with the exception of en
		passants, destPiece is always the piece that was captured. */
	inline Piece getDestPiece() const {
		return Piece(static_cast<Side>((m_Move >> 17) & 0x3),
			         static_cast<PieceType>((m_Move >> 19) & 0x7));
	}

	/** En passant square before this move was played. */
	inline Square getPrevEnPassantSquare() const { return (m_Move >> 22) & 0x7f; }

	/** The type of piece the source piece of this move is promoting to. */
	inline PieceType getPromotionTarget() const {
		return static_cast<PieceType>((m_Move >> 29) & 0x7);
	}

	/** True if this move is an en passant capture. */
	inline bool isEnPassant() {
		return (getSourcePiece().getType() == PieceType::Pawn &&
				getDest() == getPrevEnPassantSquare());
	}

	/** True if this move is castles (O-O or O-O-O) */
	inline bool isCastles() {
		bool ret = getSourcePiece().getType() == PieceType::King &&
                (std::abs(squares::fileOf(getSource()) - squares::fileOf(getDest())) == 2);

		LUNA_ASSERT(!ret || !isCapture(), "Castling is not a capture");

		return ret;
	}

	/** True if this is a castling move to a specific lateral side. */
	inline bool isCastles(LateralSide lSide) {
		if (!isCastles()) {
			return false;
		}
	
		if (lSide == LateralSide::KingSide) {
			return squares::fileOf(getDest()) > squares::fileOf(getSource());
		}
		return squares::fileOf(getDest()) < squares::fileOf(getSource());
	}

	/** True if this is a two-square push from a pawn. */
	inline bool isPawnDoublePush() {
		if (getSourcePiece().getType() != PieceType::Pawn) {
			return false;
		}

		int delta = std::abs(getDest() - getSource());
		return delta == 16;
	}

	/** True if this move is a capture. Note that en passant captures will return
		true to this, but 'getDestPiece()' will still return PIECE_NONE. */
	inline bool isCapture() const {
		if (getDestPiece() != PIECE_NONE) {
			return true;
		}

		// Only capture with no destPiece is en passant.
		// However, instead of doing complex en passant calculations,
		// we can simply return based on the assumption that all pawn moves
		// along the X axis (files) are captures.
		return (getSourcePiece().getType() == PieceType::Pawn &&
			squares::fileOf(getSource()) != squares::fileOf(getDest())); // destination file is different than source file
	}

	inline bool operator==(Move other) const {
		return this->m_Move == other.m_Move;
	}

	inline bool operator!=(Move other) const {
		return this->m_Move != other.m_Move;
	}

	Move(const Position& pos, Square src, Square dest, PieceType promotionTarget = PieceType::None);

	Move(Move&& other) = default;

	inline Move& operator=(const Move& other) {
		m_Move = other.m_Move;
		return *this;
	}

	inline Move(const Move& other)
		: m_Move(other.m_Move) {
	}

	bool invalid() const { return m_Move == 0; }

	Move() = default;

private:
	ui64 m_Move;

	inline void setSrcSquare(Square sqr) {
		m_Move &= ~0x3f; 
		m_Move |= sqr & 0x3f;
	}

	inline void setDstSquare(Square sqr) {
		m_Move &= ~(0x3f << 6);
		m_Move |= (sqr & C64(0x3f)) << 6;
	}

	inline void setSrcPiece(Piece piece) {
		m_Move &= ~(0x3 << 12);
		m_Move |= (static_cast<ui64>(piece.getSide()) & C64(0x3)) << 12;
		m_Move &= ~(0x7 << 14);
		m_Move |= (static_cast<ui64>(piece.getType()) & C64(0x7)) << 14;
	}

	inline void setDstPiece(Piece piece) {
		m_Move &= ~(0x3 << 17);
		m_Move |= (static_cast<ui64>(piece.getSide()) & C64(0x3)) << 17;
		m_Move &= ~(0x7 << 19);
		m_Move |= (static_cast<ui64>(piece.getType()) & C64(0x7)) << 19;
	}

	inline void setEpSquare(Square sqr) {
		m_Move &= ~(0x7f << 22);
		m_Move |= (sqr & C64(0x7f)) << 22;
	}

	inline void setPromotionTarget(PieceType pt) {
		m_Move &= ~(0x7 << 29);
		m_Move |= (static_cast<ui64>(pt) & C64(0x7)) << 29;
	}

	inline void setPreviousCastleRights(CastlingRightsMask crm) {
		m_Move &= ~(C64(0xf) << 32);
		m_Move |= (static_cast<ui64>(crm) & C64(0xf)) << 32;
	}
};

namespace MoveFlags {

enum {

	Quiet = 1,
	Checks = 2,
	Captures = 4,
	Noisy = Checks | Captures,
	All = Quiet | Noisy

};

}

#define MOVE_INVALID (Move::getInvalidMove())

inline std::ostream& operator<<(std::ostream& stream, Move move) {
	stream << squares::getName(move.getSource()) << squares::getName(move.getDest());
	auto promotionTarget = move.getPromotionTarget();
	if (promotionTarget != PieceType::None) {
		stream << getPieceTypePrefix(promotionTarget);
	}
	return stream;
}

}

#endif // LUNA_MOVE_H