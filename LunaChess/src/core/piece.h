#ifndef LUNA_PIECETYPE_H
#define LUNA_PIECETYPE_H

#include "types.h"
#include "debug.h"

// For loop for each piece type, except 'None' and '_Count' (the latter should only be used for iterating purposes).
#define FOREACH_PIECE_TYPE(pt) \
	for (PieceType pt = PieceType::Pawn; static_cast<int>(pt) < static_cast<int>(PieceType::_Count); \
	     pt = static_cast<PieceType>(static_cast<int>(pt) + 1))

namespace lunachess {

BITWISE_ENUM_CLASS(PieceType, ui8,

	None = 0,
	Pawn = 1,
	Knight = 2,
	Bishop = 3,
	Rook = 4,
	Queen = 5,
	King = 6,

	_Count = 7

);

char getPieceTypePrefix(PieceType type);
const char* getPieceTypeName(PieceType type);

class Piece {
public:	
	inline Side getSide() const {
		return m_Side;
	}

	inline PieceType getType() const {
		return m_Type;
	}

	int getPointValue() const;

	inline bool operator==(Piece other) const {
		return other.m_Side == m_Side && other.m_Type == m_Type;
	}

	inline bool operator!=(Piece other) const {
		return other.m_Type != m_Type || other.m_Side != m_Side;
	}

	inline Piece() = default;

	inline constexpr Piece(Side side, PieceType type)
		: m_Side(side), m_Type(type)
	{
	}

	static void initialize();

private:
	Side m_Side;
	PieceType m_Type;
};

const char* getPieceName(Piece piece);

inline static constexpr Piece WHITE_PAWN = Piece(Side::White, PieceType::Pawn);
inline static constexpr Piece WHITE_KNIGHT = Piece(Side::White, PieceType::Knight);
inline static constexpr Piece WHITE_BISHOP = Piece(Side::White, PieceType::Bishop);
inline static constexpr Piece WHITE_ROOK = Piece(Side::White, PieceType::Rook);
inline static constexpr Piece WHITE_QUEEN = Piece(Side::White, PieceType::Queen);
inline static constexpr Piece WHITE_KING = Piece(Side::White, PieceType::King);

inline static constexpr Piece BLACK_PAWN = Piece(Side::Black, PieceType::Pawn);
inline static constexpr Piece BLACK_KNIGHT = Piece(Side::Black, PieceType::Knight);
inline static constexpr Piece BLACK_BISHOP = Piece(Side::Black, PieceType::Bishop);
inline static constexpr Piece BLACK_ROOK = Piece(Side::Black, PieceType::Rook);
inline static constexpr Piece BLACK_QUEEN = Piece(Side::Black, PieceType::Queen);
inline static constexpr Piece BLACK_KING = Piece(Side::Black, PieceType::King);

inline static constexpr Piece PIECE_NONE = Piece(Side::None, PieceType::None);
}

#endif // LUNA_PIECETYPE_H
