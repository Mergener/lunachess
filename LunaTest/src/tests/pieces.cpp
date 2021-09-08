#include "../tests.h"

#include "core/piece.h"
#include "core/types.h"

namespace lunachess::tests {

void testPieces() {
	PieceType arr[6];

	int i = 0;
	FOREACH_PIECE_TYPE(pt) {
		arr[i] = pt;
		i++;
	}

	LUNA_ASSERT(arr[0] == PieceType::Pawn, "Wrong piece type in array. Expected pawn, got " << getPieceTypeName(arr[0]) << ".\n");
	LUNA_ASSERT(arr[1] == PieceType::Knight, "Wrong piece type in array. Expected knight, got " << getPieceTypeName(arr[1]) << ".\n");
	LUNA_ASSERT(arr[2] == PieceType::Bishop, "Wrong piece type in array. Expected bishop, got " << getPieceTypeName(arr[2]) << ".\n");
	LUNA_ASSERT(arr[3] == PieceType::Rook, "Wrong piece type in array. Expected rook, got " << getPieceTypeName(arr[3]) << ".\n");
	LUNA_ASSERT(arr[4] == PieceType::Queen, "Wrong piece type in array. Expected queen, got " << getPieceTypeName(arr[4]) << ".\n");
	LUNA_ASSERT(arr[5] == PieceType::King, "Wrong piece type in array. Expected king, got " << getPieceTypeName(arr[5]) << ".\n");
}

}