#include "piece.h"

namespace lunachess {

static int s_PointValues[(int)PieceType::_Count];
static char s_Prefixes[(int)PieceType::_Count];

void Piece::initialize() {
	s_PointValues[(int)PieceType::None] = 0;
	s_PointValues[(int)PieceType::Pawn] = 1;
	s_PointValues[(int)PieceType::Knight] = 3;
	s_PointValues[(int)PieceType::Bishop] = 3;
	s_PointValues[(int)PieceType::Rook] = 5;
	s_PointValues[(int)PieceType::Queen] = 9;
	s_PointValues[(int)PieceType::King] = 99999;

	s_Prefixes[(int)PieceType::None] = ' ';
	s_Prefixes[(int)PieceType::Pawn] = 'p';
	s_Prefixes[(int)PieceType::Knight] = 'n';
	s_Prefixes[(int)PieceType::Bishop] = 'b';
	s_Prefixes[(int)PieceType::Rook] = 'r';
	s_Prefixes[(int)PieceType::Queen] = 'q';
	s_Prefixes[(int)PieceType::King] = 'k';
}

int Piece::getPointValue() const {
	return s_PointValues[(int)getType()];
}

char getPieceTypePrefix(PieceType type) {
	return s_Prefixes[(int)type];
}

const char* getPieceTypeName(PieceType type) {
	switch (type) {
		case PieceType::Pawn:
			return "Pawn";
		case PieceType::Knight:
			return "Knight";
		case PieceType::Bishop:
			return "Bishop";
		case PieceType::Rook:
			return "Rook";
		case PieceType::Queen:
			return "Queen";
		case PieceType::King:
			return "King";
		default:
			return "None";
	}
}

const char* getPieceName(Piece piece) {
	if (piece.getSide() == Side::White) {
		switch (piece.getType()) {
			case PieceType::Pawn:
				return "White Pawn";
			case PieceType::Knight:
				return "White Knight";
			case PieceType::Bishop:
				return "White Bishop";
			case PieceType::Rook:
				return "White Rook";
			case PieceType::Queen:
				return "White Queen";
			case PieceType::King:
				return "White King";
			default:
				return "Empty Piece";
		}
	}
	else if (piece.getSide() == Side::Black) {
		switch (piece.getType()) {
			case PieceType::Pawn:
				return "Black Pawn";
			case PieceType::Knight:
				return "Black Knight";
			case PieceType::Bishop:
				return "Black Bishop";
			case PieceType::Rook:
				return "Black Rook";
			case PieceType::Queen:
				return "Black Queen";
			case PieceType::King:
				return "Black King";
			default:
				return "Empty Piece";
		}
	}
	else {
		return "Empty Piece";
	}

}


}