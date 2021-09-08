#include "../tests.h"

#include "core/position.h"

namespace lunachess::tests{

static struct {
	const char* fen;
	CastlingRightsMask expectedCRM;
	Side expectedSideToMove;
	Square expectedEPSquare;
	bool expectedCanOONow;
	bool expectedCanOOONow;
	bool legal;

	// Note: isCheck() only expected to return true when the king of the side to move is being attacked.
	// This means that illegal positions (current side to move attacks the opponent king) may
	// not return true (unless the side to move's king is also being attacked).
	bool isCheck;

	int legalMoveCount;
} s_Positions[] = {
	  // Initial chess position
	{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	  CastlingRightsMask::All, Side::White, SQ_INVALID, false, false, true, false },

	  // Position after 1. d4
	{ "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 1",
	  CastlingRightsMask::All, Side::Black, SQ_D3, false, false, true, false },

	  // Ruy López, Berlin Defense "1. e4 e5 2. Nf3 Nc6 3. Bb5 Nf6"
	{ "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
	  CastlingRightsMask::All, Side::White, SQ_INVALID, true, false, true, false },

	  // Black is checking the white's king with a bishop.
	{ "rnbqk1nr/pppp1ppp/8/4p3/1b1PP3/8/PPP2PPP/RNBQKBNR w KQkq - 1 3",
	  CastlingRightsMask::All, Side::White, SQ_INVALID, false, false, true, true },

	  // Same position as above, except it's black to move and the position is illegal.
	{ "rnbqk1nr/pppp1ppp/8/4p3/1b1PP3/8/PPP2PPP/RNBQKBNR b KQkq - 1 3",
	  CastlingRightsMask::All, Side::Black, SQ_INVALID, false, false, false, false },
};

static const char* boolToCStr(bool b) {
	return b ? "true" : "false";
}

void testPositions() {
	constexpr int COUNT = sizeof(s_Positions) / sizeof(*s_Positions);

	for (int i = 0; i < COUNT; ++i) {
		auto& x = s_Positions[i];

		Position pos = Position::fromFEN(x.fen).value();

		LUNA_ASSERT(pos.getCastleRightsMask() == x.expectedCRM, "Invalid CRM. (expected " << static_cast<int>(x.expectedCRM) <<
			", got " << static_cast<int>(pos.getCastleRightsMask()) << ")");


		LUNA_ASSERT(pos.getSideToMove() == x.expectedSideToMove, "Invalid side to move. (expected " << static_cast<int>(x.expectedSideToMove) <<
			", got " << static_cast<int>(pos.getSideToMove()) << ")");

		LUNA_ASSERT(pos.getEnPassantSquare() == x.expectedEPSquare,
			"Invalid En Passant square (expected " << squares::getName(x.expectedEPSquare) << 
			", got " << squares::getName(pos.getEnPassantSquare()) << ")");

		LUNA_ASSERT(pos.getEnPassantSquare() == x.expectedEPSquare,
			"Invalid 'can O-O now' status. (expected " << boolToCStr(x.expectedCanOONow) <<
			", got " << boolToCStr(pos.canCastleNow(LateralSide::KingSide)) << ")");

		LUNA_ASSERT(pos.getEnPassantSquare() == x.expectedEPSquare,
			"Invalid 'can O-O-O now' status. (expected " << boolToCStr(x.expectedCanOOONow) <<
			", got " << boolToCStr(pos.canCastleNow(LateralSide::QueenSide)) << ")");

		bool legal = pos.legal();
		LUNA_ASSERT(legal == x.legal,
			"Invalid legality. (expected " << boolToCStr(x.legal) << ", got " << boolToCStr(legal) << ")");

		if (legal) {
			// Position is legal, test make/unmake moves
			MoveList moves;
			int count = pos.getLegalMoves(moves);
			if (count == 0) {
				// No legal moves, ignore this position
				continue;
			}
			
			ui64 originalZobrist = pos.getZobristKey();
			std::string fen = pos.toFen();

			for (auto m : moves) {
				Position repl = pos;

				LUNA_ASSERT(pos.getEnPassantSquare() == m.getPrevEnPassantSquare(),
					"Move 'prevEnPassantSquare' must match the position en passant square."
					<< " (expected " << squares::getName(pos.getEnPassantSquare())
					<< ", got " << squares::getName(m.getPrevEnPassantSquare()) << ")");

				repl.makeMove(m);
				repl.undoMove(m);

				LUNA_ASSERT(pos.getZobristKey() == originalZobrist,
					"Original Zobrist key must be equal to the new key after makeMove (expected %" << originalZobrist << 
					", got %" << pos.getZobristKey() << ").\n"
					"Played move was " << m << " in position " << pos.toFen() << ".");
			}
		}
	}
}

}