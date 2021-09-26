// JEITO NOVO

#include "position.h"

#include <stdexcept>
#include <charconv>
#include <sstream>
#include <algorithm>

#include "bitboard.h"
#include "bits.h"
#include "debug.h"

namespace lunachess {

ui64 Position::perft(int depth) const {
	Position repl = *this;
	return repl.perftInternal(depth);
}

void Position::richPerft(int depth, PerftStats& stats) const {
	Position repl = *this;
	repl.richPerftInternal(depth, stats);
}

#ifdef LUNA_ASSERTS_ON
#define PERFT_BEFORE_MAKE(rep) \
	ui64 prevKey = (rep).getZobristKey(); \
	CastlingRightsMask prevCrm = (rep).getCastleRightsMask();

#define PERFT_AFTER_MAKE(rep, mvlist) \
	LUNA_ASSERT(prevKey == (rep).getZobristKey(),\
		"Keys must be the same after make/unmake. (expected " << prevKey << ", got " << (rep).getZobristKey() <<\
		" in position " << (rep).toFen() << ", move " << mvlist[i] << ")");\
\
	LUNA_ASSERT(prevCrm == (rep).getCastleRightsMask(),\
		"CRM must be the same after make/unmake. (expected " << (int)prevCrm << ", got " << (int)(rep).getCastleRightsMask() <<\
		" in position " << (rep).toFen() << ", move " << mvlist[i] << ")");
#else 
#define PERFT_BEFORE_MAKE(rep)
#define PERFT_AFTER_MAKE(rep, mvlist)
#endif

void Position::richPerftInternal(int depth, PerftStats& stats) {
	if (depth == 0) {
		stats.leafNodes++;
		return;
	}

	MoveList moves;
	int nMoves;

	nMoves = getLegalMoves(moves);

	for (int i = 0; i < nMoves; i++) {
		auto move = moves[i];

		PERFT_BEFORE_MAKE(*this);

		if (move.getPromotionTarget() != PieceType::None) {
			stats.promotions++;
		}

		if (move.isCapture()) {
			stats.captures++;
			if (move.isEnPassant()) {
				stats.enPassants++;
			}
		}
		else if (move.isCastles()) {
			stats.castles++;
		}

		makeMove(move);
		richPerftInternal(depth - 1, stats);
		undoMove(move);

		PERFT_AFTER_MAKE(*this, moves);
	}
}

ui64 Position::perftInternal(int depth) {
	MoveList moves;
	int nMoves;
	ui64 nodes = 0;

	nMoves = getLegalMoves(moves);

	if (depth == 1) {
		return nMoves;
	}

	for (int i = 0; i < nMoves; i++) {
		PERFT_BEFORE_MAKE(*this);

		makeMove(moves[i]);
		nodes += perftInternal(depth - 1);
		undoMove(moves[i]);

		PERFT_AFTER_MAKE(*this, moves);
	}

	return nodes;
}

int Position::getMaterialCount(Side side) const {
    int count = 0;

    FOREACH_PIECE_TYPE(pt) {
        Piece piece = Piece(side, pt);
        auto bb = getPieceBitboard(piece);

        count += bb.count() * piece.getPointValue();
    }

    return count;
}

bool Position::sideHasSufficientMaterial(Side side) const {
	// Check for heavy pieces and pawns
	Bitboard heavyBB = getPieceBitboard(PieceType::Rook, side) &
		getPieceBitboard(PieceType::Queen, side) &
		getPieceBitboard(PieceType::Pawn, side);

	if (heavyBB.count() > 0) {
		return false;
	}

	// Check for lesser pieces
	Bitboard lightBB = getPieceBitboard(PieceType::Bishop, side) &
		getPieceBitboard(PieceType::Knight, side);
	if (lightBB.count() > 1) {
		return false;
	}

	return true;
}

std::string Position::toFen() const {
	std::stringstream stream;

	int emptySquares = 0;
	for (int j = 7; j >= 0; --j)
	{
		for (int i = 0; i < 8; ++i)
		{
			Square sq = j * 8 + i;
			Piece piece = getPieceAt(sq);

			if (piece == PIECE_NONE)
			{
				emptySquares++;
			}
			else
			{
				if (emptySquares > 0)
				{
					stream << emptySquares;
					emptySquares = 0;
				}
				char prefix = getPieceTypePrefix(piece.getType());
				prefix = piece.getSide() == Side::White ? std::toupper(prefix) : std::tolower(prefix);
				stream << prefix;
			}
		}
		if (emptySquares > 0)
		{
			stream << emptySquares;
			emptySquares = 0;
		}

		if (j != 0) {
			stream << '/';
		}
	}

	stream << ' ' << (getSideToMove() == Side::White ? 'w' : 'b') << ' ';

	bool hasAnyCastlingRights = false;
	if (getCastleRights(Side::White, LateralSide::KingSide))
	{
		hasAnyCastlingRights = true;
		stream << 'K';
	}
	if (getCastleRights(Side::White, LateralSide::QueenSide))
	{
		hasAnyCastlingRights = true;
		stream << 'Q';
	}
	if (getCastleRights(Side::Black, LateralSide::KingSide))
	{
		hasAnyCastlingRights = true;
		stream << 'k';
	}
	if (getCastleRights(Side::Black, LateralSide::QueenSide))
	{
		hasAnyCastlingRights = true;
		stream << 'q';
	}

	if (!hasAnyCastlingRights)
	{
		stream << '-';
	}

	stream << ' ';

	if (getEnPassantSquare() != SQ_INVALID)
	{
		stream << squares::getName(getEnPassantSquare());
	}
	else
	{
		stream << '-';
	}

	stream << ' ' << get50moveRuleCounter() << ' ' << getPlyCounter() / 2 + 1;

	return stream.str();
}

std::optional<Position> Position::fromFEN(std::string_view fen) {
	// Remove space duplicates
	std::string fenStr(fen);

	std::string::iterator newEnd = std::unique(fenStr.begin(), fenStr.end(), [](char a, char b) { 
		return (a == b) && (a == ' '); 
	});
	fenStr.erase(newEnd, fenStr.end());
	fen = fenStr;

	Position pos;
	pos.setCastleRights(CastlingRightsMask::None);

	int i = 0;
	int rank = 7;
	int file = 0;
	for (; fen[i] != ' '; ++i) {
		if (rank < 0) {
			return std::nullopt;
		}
		Square sq = (rank * 8) + (file % 8);
		char c = fen[i];
		Side s = Side::None;
		PieceType p = PieceType::None;

		if (c == '/') {
			// Go to next rank
			rank -= 1;
			file = 0;
			continue;
		}

		if (std::isdigit(c)) {
			// A digit has been found, skip some squares horizontally
			int skip = c - '0';

			file += skip;

			continue;
		}

		switch (c) {
		case 'r':
			s = Side::Black;
			p = PieceType::Rook;
			break;

		case 'n':
			s = Side::Black;
			p = PieceType::Knight;
			break;

		case 'b':
			s = Side::Black;
			p = PieceType::Bishop;
			break;

		case 'q':
			s = Side::Black;
			p = PieceType::Queen;
			break;

		case 'k':
			s = Side::Black;
			p = PieceType::King;
			break;

		case 'p':
			s = Side::Black;
			p = PieceType::Pawn;
			break;

		case 'R':
			s = Side::White;
			p = PieceType::Rook;
			break;

		case 'N':
			s = Side::White;
			p = PieceType::Knight;
			break;

		case 'B':
			s = Side::White;
			p = PieceType::Bishop;
			break;

		case 'Q':
			s = Side::White;
			p = PieceType::Queen;
			break;

		case 'K':
			s = Side::White;
			p = PieceType::King;
			break;

		case 'P':
			s = Side::White;
			p = PieceType::Pawn;
			break;

		default:
			return std::nullopt;
		}

		pos.setPieceAt(sq, Piece(s, p));
		file++;
	}


	i++; // Skip a space

	// Get side to move
	pos.setSideToMove(fen[i] == 'w' ? Side::White : Side::Black);

	i += 2; // Skip a space

	// Check castling availability
	if (fen[i] != '-') {
		while (fen[i] != ' ') {
			switch (fen[i]) {
			case 'k':
				pos.setCastleRights(Side::Black, LateralSide::KingSide, true);
				break;

			case 'K':
				pos.setCastleRights(Side::White, LateralSide::KingSide, true);
				break;

			case 'q':
				pos.setCastleRights(Side::Black, LateralSide::QueenSide, true);
				break;

			case 'Q':
				pos.setCastleRights(Side::White, LateralSide::QueenSide, true);
				break;
			}

			i++;
		}
	}
	else {
		i++;
	}

	i++; // Skip a space

	// Get en passant square
	if (fen[i] != '-') {
		// There is an en passant square
		std::string_view squareStr = fen.substr(i, 2);
		i += 2;
		Square eps = squares::fromStr(squareStr);
		if (eps == SQ_INVALID) {
			return std::nullopt;
		}
		pos.setEnPassantSquare(eps);
	}
	else {
		pos.setEnPassantSquare(SQ_INVALID);
		i++;
	}

	i++; // Skip a space
	if (i >= fen.size()) {
		return pos;
	}

	// Get Halfmove clock
	int halfMoveClockStrLen = 0;
	int halfMoveClockStrBegin = i;
	while (std::isdigit(fen[i])) {
		halfMoveClockStrLen++;
		i++;
	}

	if (halfMoveClockStrLen > 0) {
		auto substr = fen.substr(halfMoveClockStrBegin, halfMoveClockStrLen);
		auto res = std::from_chars(substr.data(), substr.data() + substr.size(), pos.m_50MoveClock);
		if (res.ec == std::errc::invalid_argument || res.ec == std::errc::result_out_of_range) {
			return std::nullopt;
		}
	}

	return pos;
}

bool Position::getCastleRights(Side side, LateralSide lSide) const {
	ui8 bit = 1 << ((static_cast<ui8>(side) - 1) * 2 + static_cast<ui8>(lSide));

	return (m_CastleMask & bit) != CastlingRightsMask::None;
}

void Position::setCastleRights(CastlingRightsMask crm) {
	m_Zobrist ^= zobrist::getCastlingRightsKey(m_CastleMask);
	m_CastleMask = crm;
	m_Zobrist ^= zobrist::getCastlingRightsKey(m_CastleMask);
}

void Position::setCastleRights(Side side, LateralSide lSide, bool allow) {
	// Get the bit to be set/unset
	const ui8 bit = 1 << ((static_cast<ui8>(side) - 1) * 2 + static_cast<ui8>(lSide));

	CastlingRightsMask crm = m_CastleMask;
	
	if (allow) {
		crm |= bit;
	}
	else {
		crm &= ~bit;
	}
	
	setCastleRights(crm);
}

bool Position::legal() const {
	// Position is legal if we cannot 'capture' opponent king
	Side side = getSideToMove();
	Square opponentKingPos = getKingSquare(getOppositeSide(side));
	Bitboard occupancy = getCompositeBitboard();

	for (Square s = 0; s < 64; ++s) {
		auto p = m_Squares[s];
		if (p.getSide() != side) {
			continue;
		}

		auto attacks = bitboards::getPieceAttacks(p.getType(), occupancy, s, side);
		if (attacks.contains(opponentKingPos)) {
			return false;
		}
	}

	return true;
}

bool Position::moveIsPseudoLegal(Move move) const {
	Bitboard occ = getPieceBitboard(move.getSourcePiece());

	MoveList l;
	getPseudoLegalMoves(l);

	return l.contains(move);
}

static int s_PromotionRank[] = { 7, 0 };

int Position::getPseudoLegalMoves(MoveList& l, ui64 moveFlagsMask) const {
	const int startingCount = l.count();

	Side sideToMove = getSideToMove();

	FOREACH_PIECE_TYPE(pt) {
		auto piece = Piece(sideToMove, pt);
		auto bb = getPieceBitboard(Piece(sideToMove, pt));
		auto occ = getCompositeBitboard();

		if (pt == PieceType::Pawn) {
			// Add the en passant square to the occupancy squares.
			// This is a simple trick to allow captures at en passant squares.
			auto eps = getEnPassantSquare();
			if (eps != SQ_INVALID) {
				occ.add(getEnPassantSquare());
			}
		}

		for (auto s : bb) {
			// Get piece attacks
			auto attacks = bitboards::getPieceAttacks(piece.getType(), occ, s, sideToMove);

			if (piece.getType() == PieceType::Pawn) {
				int promotionRank = s_PromotionRank[(int)sideToMove - 1];

				for (Square as : attacks) {
					auto target = getPieceAt(as);
					if (target.getSide() == piece.getSide()) {
						// Cannot attack a piece of the same color
						continue;
					}

					if (squares::rankOf(as) == promotionRank) {
						// Can only move to promote
						l.add(Move(*this, s, as, PieceType::Bishop));
						l.add(Move(*this, s, as, PieceType::Rook));
						l.add(Move(*this, s, as, PieceType::Queen));
						l.add(Move(*this, s, as, PieceType::Knight));
					}
					else {
						// Cannot promote
						l.add(Move(*this, s, as, PieceType::None));
					}
				}
			}
			else {
				// Not a pawn, add all attacks that don't target a friendly occupied square
				for (Square as : attacks) {
					auto target = getPieceAt(as);
					if (target.getSide() == piece.getSide()) {
						// Cannot attack a piece of the same color
						continue;
					}
					l.add(Move(*this, s, as));
				}
			}
		}
	}

	if (canCastleNow(LateralSide::KingSide)) {
		l.add(Move(*this, squares::getKingDefaultSquare(sideToMove), squares::getCastleKingDestSquare(sideToMove, LateralSide::KingSide)));
	}
	if (canCastleNow(LateralSide::QueenSide)) {
		l.add(Move(*this, squares::getKingDefaultSquare(sideToMove), squares::getCastleKingDestSquare(sideToMove, LateralSide::QueenSide)));
	}

	return l.count() - startingCount;
}

bool Position::canCastleNow(LateralSide lateralSide) const {
	auto side = getSideToMove();

	if (!getCastleRights(side, lateralSide)) {
		// Cannot castle if castling rights for a specific side were
		// already revoked.
		return false;
	}

	// We have castling rights. Now, we need to see if the castling
	// path is being attacked by opponent pieces.
	// Note that the castling path already includes the king's square.
	auto opponent = getOppositeSide(side);

	auto occupancy = getCompositeBitboard();
	auto kingSqr = getKingSquare(side);

	LUNA_ASSERT(kingSqr == squares::getKingDefaultSquare(side),
		"Castle rights exist even though a king is not on their original square. (king is on " << squares::getName(kingSqr) <<
		", original square is " << squares::getName(squares::getKingDefaultSquare(side)) << ").\nPosition: " << toFen());

	auto rookSqr = squares::getCastleRookSquare(side, lateralSide);

	// We don't want to count the king nor the rook as obstacles
	// on the path to castling
	occupancy.remove(kingSqr);
	occupancy.remove(rookSqr);

	auto kingPath = bitboards::getCastlingKingPath(side, lateralSide);
	auto rookPath = bitboards::getCastlingRookPath(side, lateralSide);

	if (occupancy & (kingPath | rookPath)) {
		// Something is in the way of our king, cannot castle now.
		return false;
	}

	// Cannot castle if an opponent piece attacks our king's
	// castling path.
	for (auto pathSqr : kingPath) {
		// For every opponent piece bitboard
		FOREACH_PIECE_TYPE(pt) {
			auto bb = getPieceBitboard(static_cast<PieceType>(pt), opponent);

			// For every piece in the bitboard
			for (auto s : bb) {
				auto opponentPiece = getPieceAt(s);

				auto attacks = bitboards::getPieceAttacks(opponentPiece.getType(), occupancy, s, opponent);
				if (attacks.contains(pathSqr)) {
					// An opponent piece attacks our castling square, castling is impossible.
					return false;
				}
			}
		}
	}

	// No piece is obstructing the king, neither an opponent piece
	// is attacking his castling path. Also, castling rights still hold.
	// Castling is allowed.
	return true;
}

Square Position::getKingSquare(Side side) const {
	Bitboard kbb = getPieceBitboard(PieceType::King, side);
	Square sq = SQ_INVALID;
	bits::bitScanF(kbb, sq);
	return sq;
}

bool Position::isCheck() const {
	auto occ = getCompositeBitboard();
	auto sideToMove = getSideToMove();
	auto oppositeSide = getOppositeSide(sideToMove);
	Square kingSquare = getKingSquare(sideToMove);

	FOREACH_PIECE_TYPE(pt) {
		auto bb = getPieceBitboard(pt, oppositeSide);

		for (Square ps : bb) {
			auto attacks = bitboards::getPieceAttacks(pt, occ, ps, oppositeSide);
			if (attacks.contains(kingSquare)) {
				return true;
			}
		}
	}

	return false;
}

int Position::getLegalMoves(MoveList& ret, ui64 moveFlagsMask) const {
	const int idx = ret.count(); // index to start placing elements at movelist
	const int count = getPseudoLegalMoves(ret, moveFlagsMask); // pseudo-moves list cound

	// Note: the move list could already be filled with some moves.

	Position replica = *this;
	int removed = 0;
	for (int i = idx + count - 1; i >= idx; --i) {
		const Move move = ret[i];

		if ((moveFlagsMask == MoveFlags::Captures) && !move.isCapture()) {
			// Non-capture moves not accepted
			ret.removeAt(i);
			removed++;
			continue;
		}

		replica.makeMove(move, false);
		if (!replica.legal()) {
			// Move leads to illegal position, therefore it is not
			// a legal move.
			ret.removeAt(i);
			removed++;
		}
		replica.undoMove(move);
	}

	return count - removed;
}

void Position::handleSpecialMove(Move move) {
	if (move.isEnPassant()) {
		// Get the 'push' direction of the pawn that made the move.
		int pushDir = bitboards::getPawnPushStep(move.getSourcePiece().getSide());
		Square captureSq = move.getDest() - pushDir;
		setPieceAt(captureSq, PIECE_NONE);
	}
	else if (move.isPawnDoublePush()) {
		int pushStep = bitboards::getPawnPushStep(move.getSourcePiece().getSide());

		setEnPassantSquare(move.getDest() - pushStep);
	}
	else if (move.isCastles(LateralSide::KingSide)) {
		auto king = move.getSourcePiece();
		int rank = squares::rankOf(move.getSource());
		Square prevRookSq = (rank * 8) + 7;
		Square newRookSq = (rank * 8) + 5;

		setPieceAt(newRookSq, Piece(king.getSide(), PieceType::Rook));
		setPieceAt(prevRookSq, Piece(king.getSide(), PieceType::None));
	}
	else if (move.isCastles(LateralSide::QueenSide)) {
		auto king = move.getSourcePiece();
		int rank = squares::rankOf(move.getSource());
		Square prevRookSq = (rank * 8);
		Square newRookSq = (rank * 8) + 3;

		setPieceAt(newRookSq, Piece(king.getSide(), PieceType::Rook));
		setPieceAt(prevRookSq, Piece(king.getSide(), PieceType::None));
	}
}

bool Position::isRepetitionDraw(int maxRepetitions) const {
	int reps = 1;
	for (int i = m_DrawList.count() - 1; i >= 0; --i) {
		auto key = m_DrawList[i];

		if (key == m_Zobrist) {
			if (reps == (maxRepetitions)) {
				return true;
			}
			reps++;
		}
	}
	return false;
}

bool Position::makeMove(Move move, bool validate, bool fullyReversibleDrawList) {
	if (!validate || moveIsLegal(move)) {
		m_Ply++;

		// Move is being made
		m_DrawList.add(m_Zobrist);

		setEnPassantSquare(SQ_INVALID); // Double forward pawn moves will override this line 
		
		// Invert current side to move
		setSideToMove(getOppositeSide(getSideToMove()));

		auto sourcePiece = move.getSourcePiece();
		auto sourcePieceType = sourcePiece.getType();
		auto sourcePieceSide = sourcePiece.getSide();

		if (sourcePieceType == PieceType::Pawn) {
			// Pawn moves always reset 50 move clock
			m_50MoveClock = 0;
			
			if (!fullyReversibleDrawList) {
				m_DrawList.clear();
			}

			if (move.getPromotionTarget() != PieceType::None) {
				// Pawn is willing to promote, a new piece is being placed
				// in the target square.
				setPieceAt(move.getDest(), Piece(sourcePieceSide, move.getPromotionTarget()));
			}
			else {
				// Pawn is not willing to promote, simply put it on the target square
				setPieceAt(move.getDest(), sourcePiece);
			}
		}
		else if (move.isCapture()) {
			// Capture moves always reset 50 move clock
			m_50MoveClock = 0;

			if (!fullyReversibleDrawList) {
				m_DrawList.clear();
			}

			setPieceAt(move.getDest(), sourcePiece);
		}
		else {
			// Move is not a capture nor a pawn move
			m_50MoveClock++;
			setPieceAt(move.getDest(), sourcePiece);
		}
		setPieceAt(move.getSource(), PIECE_NONE);

		// Check if we might have lost our castling rights on this move
		if (sourcePieceType == PieceType::King) {
			// King moves always remove both castling rights
			setCastleRights(sourcePieceSide, LateralSide::KingSide, false);
			setCastleRights(sourcePieceSide, LateralSide::QueenSide, false);
		}
		else if (sourcePieceType == PieceType::Rook) {
			// Rook moves remove their respective lateral side castling rights
			if (move.getSource() == SQ_A1 || move.getSource() == SQ_A8) {
				setCastleRights(sourcePieceSide, LateralSide::QueenSide, false);
			}
			else if (move.getSource() == SQ_H1 || move.getSource() == SQ_H8) {
				setCastleRights(sourcePieceSide, LateralSide::KingSide, false);
			}
		}

		handleSpecialMove(move);


		return true;
	}

	return false;
}

bool Position::moveIsLegal(Move move) const {
	if (!moveIsPseudoLegal(move)) {
		return false;
	}

	Position replica = *this;
	replica.makeMove(move);

	return replica.legal();
}

void Position::castleUndoHandler(LateralSide castlingSide) {
	Side side = getSideToMove();

	Square rookSq = squares::getCastleRookDestSquare(side, castlingSide);
	setPieceAt(rookSq, PIECE_NONE);

	Square originalRookSq = squares::getCastleRookSquare(side, castlingSide);
	setPieceAt(originalRookSq, Piece(side, PieceType::Rook));
}

void Position::handleSpecialMoveUndo(Move move) {
	if (move.isEnPassant()) {
		// En passant moves must replace the captured pawn back when undone
		auto epSquare = move.getPrevEnPassantSquare();
		Side capturedPawnSide = getOppositeSide(move.getSourcePiece().getSide());
		Square capturedPawnSquare = epSquare + bitboards::getPawnPushStep(capturedPawnSide);
		setPieceAt(capturedPawnSquare, Piece(capturedPawnSide, PieceType::Pawn));
	}
	// Castling moves should remove the newly added rooks and add the previous ones back
	else if (move.isCastles(LateralSide::KingSide)) {
		castleUndoHandler(LateralSide::KingSide);
	}
	else if (move.isCastles(LateralSide::QueenSide)) {
		castleUndoHandler(LateralSide::QueenSide);
	}
}

void Position::undoMove(Move move) {
	m_Ply--;

	// Recover previous castling rights
	setCastleRights(move.getPreviousCastleRights());

	// Recover previous side to move
	setSideToMove(getOppositeSide(getSideToMove()));

	// Recover previous piece placement
	setPieceAt(move.getSource(), move.getSourcePiece());
	setPieceAt(move.getDest(), move.getDestPiece());

	// Recover previous en passant square
	setEnPassantSquare(move.getPrevEnPassantSquare());

	// For special moves (e.g. castling, en passant),
	// do what's necessary to backtrack them.
	handleSpecialMoveUndo(move);

	if (m_DrawList.count() > 0) {
		m_DrawList.removeAt(m_DrawList.count() - 1);
	}
}

void Position::setPieceAt(Square sqr, Piece piece) {
	auto prevPiece = getPieceAt(sqr);

	if (prevPiece != PIECE_NONE) {
		// We had a piece on the square before,
		// update its bitboard and the zobrist key.
		auto& prevBb = getPieceBitboard(prevPiece);
		prevBb.remove(sqr);

		// Update zobrist key
		m_Zobrist ^= zobrist::getPieceSquareKey(prevPiece, sqr);
	}

	// Update grid
	m_Squares[sqr] = piece;

	if (piece != PIECE_NONE) {
		// We're placing a new piece on this square,
		// update its bitboard and the zobrist key.
		m_CompositeBitboard.add(sqr);

		// Update this piece's bitboard
		auto& bb = getPieceBitboard(piece);
		bb.add(sqr);

		// Update zobrist key
		m_Zobrist ^= zobrist::getPieceSquareKey(piece, sqr);
	}
	else {
		m_CompositeBitboard.remove(sqr);
	}
}

Position Position::getInitialPosition(bool castlingAllowed) {
	Position ret;

	// White pieces:
	ret.setPieceAt(SQ_A1, WHITE_ROOK);
	ret.setPieceAt(SQ_B1, WHITE_KNIGHT);
	ret.setPieceAt(SQ_C1, WHITE_BISHOP);
	ret.setPieceAt(SQ_D1, WHITE_QUEEN);
	ret.setPieceAt(SQ_E1, WHITE_KING);
	ret.setPieceAt(SQ_F1, WHITE_BISHOP);
	ret.setPieceAt(SQ_G1, WHITE_KNIGHT);
	ret.setPieceAt(SQ_H1, WHITE_ROOK);

	ret.setPieceAt(SQ_A2, WHITE_PAWN);
	ret.setPieceAt(SQ_B2, WHITE_PAWN);
	ret.setPieceAt(SQ_C2, WHITE_PAWN);
	ret.setPieceAt(SQ_D2, WHITE_PAWN);
	ret.setPieceAt(SQ_E2, WHITE_PAWN);
	ret.setPieceAt(SQ_F2, WHITE_PAWN);
	ret.setPieceAt(SQ_G2, WHITE_PAWN);
	ret.setPieceAt(SQ_H2, WHITE_PAWN);

	// Black pieces:
	ret.setPieceAt(SQ_A7, BLACK_PAWN);
	ret.setPieceAt(SQ_B7, BLACK_PAWN);
	ret.setPieceAt(SQ_C7, BLACK_PAWN);
	ret.setPieceAt(SQ_D7, BLACK_PAWN);
	ret.setPieceAt(SQ_E7, BLACK_PAWN);
	ret.setPieceAt(SQ_F7, BLACK_PAWN);
	ret.setPieceAt(SQ_G7, BLACK_PAWN);
	ret.setPieceAt(SQ_H7, BLACK_PAWN);

	ret.setPieceAt(SQ_A8, BLACK_ROOK);
	ret.setPieceAt(SQ_B8, BLACK_KNIGHT);
	ret.setPieceAt(SQ_C8, BLACK_BISHOP);
	ret.setPieceAt(SQ_D8, BLACK_QUEEN);
	ret.setPieceAt(SQ_E8, BLACK_KING);
	ret.setPieceAt(SQ_F8, BLACK_BISHOP);
	ret.setPieceAt(SQ_G8, BLACK_KNIGHT);
	ret.setPieceAt(SQ_H8, BLACK_ROOK);

	ret.setCastleRights(Side::White, LateralSide::KingSide, castlingAllowed);
	ret.setCastleRights(Side::White, LateralSide::QueenSide, castlingAllowed);
	ret.setCastleRights(Side::Black, LateralSide::KingSide, castlingAllowed);
	ret.setCastleRights(Side::Black, LateralSide::QueenSide, castlingAllowed);

	ret.setEnPassantSquare(SQ_INVALID);

	return ret;
}

int Position::countTotalPointValue(bool includePawns) const {
	int total = 0;

	FOREACH_SIDE(side) {
		FOREACH_PIECE_TYPE(pt) {
			if (pt == PieceType::King) {
				continue;
			}

			if (pt == PieceType::Pawn && !includePawns) {
				continue;
			}

			Piece piece = Piece(side, pt);
			Bitboard pieceBB = getPieceBitboard(piece);

			total += pieceBB.count() * piece.getPointValue();
		}
	}

	return total;
}

std::ostream& operator<<(std::ostream& stream, const Position& pos) {
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			Square sq = (7 - i) * 8 + j;

			const char* s;
			auto piece = pos.getPieceAt(sq);
			auto side = piece.getSide();
			switch (piece.getType()) {
			default:
				s = ".";
				break;
			case PieceType::Pawn:
				s = side == Side::White ? "P" : "p";
				break;
			case PieceType::Knight:
				s = side == Side::White ? "N" : "n";
				break;
			case PieceType::Bishop:
				s = side == Side::White ? "B" : "b";
				break;
			case PieceType::Rook:
				s = side == Side::White ? "R" : "r";
				break;
			case PieceType::Queen:
				s = side == Side::White ? "Q" : "q";
				break;
			case PieceType::King:
				s = side == Side::White ? "K" : "k";
				break;
			}

			stream << s << " ";
		}
		stream << std::endl;
	}
	stream << "Side to move: " << (pos.getSideToMove() == Side::White ? "White" : "Black") << std::endl;
	auto eps = pos.getEnPassantSquare();
	if (eps != SQ_INVALID) {
		stream << "En passant square: " << squares::getName(eps) << std::endl;
	}
	stream << "Legal moves: ";
	MoveList moves;
	pos.getLegalMoves(moves);
	for (int i = 0; i < moves.count(); ++i) {
		stream << moves[i];

		if (i < moves.count() - 1) {
			stream << ", ";
		}
		else {
			stream << ".\n";
		}
	}

	stream << "Castling rights: ";
	if (pos.getCastleRights(Side::White, LateralSide::KingSide)) {
		stream << "K";
	}
	if (pos.getCastleRights(Side::White, LateralSide::QueenSide)) {
		stream << "Q";
	}
	if (pos.getCastleRights(Side::Black, LateralSide::KingSide)) {
		stream << "k";
	}
	if (pos.getCastleRights(Side::Black, LateralSide::QueenSide)) {
		stream << "q";
	}
	stream << "\nCan castle now: ";
	if (pos.canCastleNow(LateralSide::KingSide)) {
		stream << (pos.getSideToMove() == Side::White ? "K" : "k");
	}
	if (pos.canCastleNow(LateralSide::QueenSide)) {
		stream << (pos.getSideToMove() == Side::White ? "Q" : "q");
	}

	stream << "\nZobrist key: " << pos.getZobristKey() << std::endl;

	stream << std::endl;

	return stream;
}

}