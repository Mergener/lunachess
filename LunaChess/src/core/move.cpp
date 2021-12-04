#include "move.h"

#include "position.h"

namespace lunachess {

Move::Move(const Position& pos, Square src, Square dest, PieceType promotionTarget)
	: m_Move(0) {
	LUNA_ASSERT(pos.getPieceAt(src) != PIECE_NONE,
		"Invalid source piece '" << getPieceName(pos.getPieceAt(src)) <<
		"' (source square " << squares::getName(src) << ", dest square " << squares::getName(dest) << ", position '" << pos.toFen() << "')")

	setSrcSquare(src);
	setDstSquare(dest);

	setSrcPiece(pos.getPieceAt(src));
	setDstPiece(pos.getPieceAt(dest));

	setEpSquare(pos.getEnPassantSquare());
	setPromotionTarget(promotionTarget);
	setPreviousCastleRights(pos.getCastleRightsMask());

	LUNA_ASSERT(getPrevEnPassantSquare() == pos.getEnPassantSquare(), "En passant squares must match in move. " <<
		"(move " << *this << " (" << m_Move << "), position fen '" << pos.toFen() << "', expected EPS " << pos.getEnPassantSquare() <<
		", got " << getPrevEnPassantSquare() << ")")
}

Move Move::fromLongAlgebraic(const Position& pos, std::string_view s) {
	if (s.size() != 4 && s.size() != 5) {
		return MOVE_INVALID;
	}

	Square src = squares::fromStr(s.substr(0, 2));
	if (src == SQ_INVALID) {
		return MOVE_INVALID;
	}
	Square dest = squares::fromStr(s.substr(2, 2));
	if (dest == SQ_INVALID) {
		return MOVE_INVALID;
	}

	PieceType p = PieceType::None;
	if (s.size() == 5) {
		switch (s[4]) {
			case 'q':
				p = PieceType::Queen;
				break;

			case 'r':
				p = PieceType::Rook;
				break;

			case 'b':
				p = PieceType::Bishop;
				break;

			case 'n':
				p = PieceType::Knight;
				break;

			default:
				return MOVE_INVALID;
		}
	}

	return Move(pos, src, dest, p);
}

}