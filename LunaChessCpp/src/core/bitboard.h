#ifndef LUNA_BITBOARD_H
#define LUNA_BITBOARD_H

#include <ostream>
#include <initializer_list>

#include "defs.h"
#include "square.h"
#include "types.h"
#include "piece.h"

namespace lunachess {

/** 
	Abstraction for uint64s that can be used as bitsets for squares on a chessboard.
*/
class Bitboard {
public:
	inline operator ui64() const {
		return m_BB;
	}

	inline bool contains(Square square) const {
		return m_BB & (C64(1) << square);
	}

	/** Union of bitboards. */
	inline Bitboard& operator|=(Bitboard other) {
		m_BB |= other.m_BB;
		return *this;
	}

	/** Intersection of bitboards. */
	inline Bitboard& operator&=(Bitboard other) {
		m_BB &= other.m_BB;
		return *this;
	}

	inline Bitboard operator<<(int n) const {
		return m_BB << n;
	}

	inline Bitboard& operator<<=(int n) {
		m_BB <<= n;
		return *this;
	}

	inline Bitboard operator>>(int n) {
		return m_BB >> n;
	}

	inline Bitboard& operator>>=(int n) {
		m_BB >>= n;
		return *this;
	}

	/** Sets the bit of the specified square to zero. */
	inline void remove(Square sq) {
		m_BB &= ~(C64(1) << sq);
	}

	/** Sets the bit of the specified square to one. */
	inline void add(Square sq) {
		m_BB |= (C64(1) << sq);
	}

	/** Returns the amount of squares in this bitboard. O(1) operation. */
	int count() const;

	inline Bitboard() 
		: m_BB(0) { }

	inline Bitboard(ui64 i)
		: m_BB(i) { }

	inline Bitboard& operator=(ui64 i) {
		m_BB = i;
		return *this;
	}

	Bitboard(const std::initializer_list<Square>& squares) {
		m_BB = 0;
		for (auto sq : squares) {
			add(sq);
		}
	}

	class Iterator {
	public:
		Iterator(const Bitboard& board, i8 bit = 0);
		Iterator(const Iterator& it) noexcept;
		Iterator(Iterator&& it) noexcept;

		Iterator& operator++();
		bool operator==(const Iterator& it);
		bool operator!=(const Iterator& it);
		Square operator*();

	private:
		ui64 m_BB;
		i8 m_Bit;
	};

	inline Iterator cbegin() const { return Iterator(*this); }
	inline Iterator cend() const { return Iterator(*this, 64); }
	inline Iterator begin() { return cbegin(); }
	inline Iterator end() { return cend(); }

private:
	ui64 m_BB;
};

std::ostream& operator<<(std::ostream& stream, Bitboard bitboard);

namespace bitboards {

// Reference for bitboards:
//	https://www.chessprogramming.org/Classical_Approach
//	https://www.chessprogramming.org/Rays

/**
	Returns 8 (>0) if side == white and -8 (<0) if side == black.
*/
inline int getPawnPushStep(Side side) {
	return side == Side::White ? 8 : -8;
}

inline int getPawnPushDir(Side side) {
	return side == Side::White ? 1 : -1;
}

/** Attack bitboards */

Bitboard getDiagonalAttacks(Bitboard occupancy, Square sqr);
Bitboard getAntiDiagonalAttacks(Bitboard occupancy, Square sqr);
Bitboard getFileAttacks(Bitboard occupancy, Square sqr);

Bitboard getKnightAttacks(Square sqr);
Bitboard getKnightAttacks(Bitboard occupancy, Square sqr, Side side);
Bitboard getRankAttacks(Bitboard occupancy, Square sqr);
Bitboard getRookAttacks(Bitboard occupancy, Square sqr, Side side);
Bitboard getBishopAttacks(Bitboard occupancy, Square sqr, Side side);
Bitboard getQueenAttacks(Bitboard occupancy, Square sqr, Side side);
Bitboard getPawnAttacks(Bitboard occupancy, Square sqr, Side side);
Bitboard getKingAttacks(Bitboard occupancy, Square sqr, Side side);

/**
	Given a square, an occupancy bitboard, a piece type and a side, returns a bitboard
	that contains all the squares the specified piece would be able to attack, assuming it
	stands at the specified square and assuming the occupancy bitboard is filled only by
	opponent pieces.
*/
Bitboard getPieceAttacks(PieceType pieceType, Bitboard occupancy, Square sqr, Side side);
inline Bitboard getPieceAttacks(Piece piece, Bitboard occupancy, Square sqr) {
	return getPieceAttacks(piece.getType(), occupancy, sqr, piece.getSide());
}

/** Other bitboards */

/**
	For a given side and lateral side, retrieves a bitboard containing the squares
	that a king would pass by when castling. Includes the king's initial square (E1 or E8).
*/
Bitboard getCastlingKingPath(Side side, LateralSide lSide);

/**
	For a given side and lateral side, retrieves a bitboard containing the squares
	that a rook would pass by when castling. Includes the rook's initial square (A1/A8/H1/H8).
*/
Bitboard getCastlingRookPath(Side side, LateralSide lSide);

Bitboard getFileBitboard(int file);
Bitboard getRankBitboard(int rank);

/** 
	Bitboard containing all light squares. SQ_A1 is a light square.
*/
Bitboard getLightSquares();

/**
	Bitboard containing all light squares. SQ_A1 is not a dark square.
*/
Bitboard getDarkSquares();

inline bool isLightSquare(Square s) {
	return getLightSquares().contains(s);
}

inline bool isDarkSquare(Square s) {
	return getDarkSquares().contains(s);
}

inline Bitboard getCastleRookSquares() {
	constexpr ui64 CASTLE_ROOK_SQUARES =
		(C64(1) << SQ_A1) |
		(C64(1) << SQ_H1) |
		(C64(1) << SQ_A8) |
		(C64(1) << SQ_H8);

	return CASTLE_ROOK_SQUARES;
}

void initialize();

} // namespace bitboards

} // namespace lunachess

#endif // LUNA_BITBOARD_H
