#include "move.h"

#include "position.h"

namespace lunachess {

Move::Move(const Position& pos, Square src, Square dest, PieceType promotionTarget)
	: m_Move(0) {
	LUNA_ASSERT(pos.getPieceAt(src) != PIECE_NONE,
		"Invalid source piece " << getPieceName(pos.getPieceAt(src)) << 
		"(source square " << squares::getName(src) << ", dest square " << squares::getName(dest) << ")")

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

}