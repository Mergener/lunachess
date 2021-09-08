#include "movepicker.h"

#include <algorithm>

#include "../core/position.h"

namespace lunachess::ai {

static bool compareCaptures(Move a, Move b) {
	int aDelta = a.getDestPiece().getPointValue() - a.getSourcePiece().getPointValue();
	int bDelta = b.getDestPiece().getPointValue() - b.getSourcePiece().getPointValue();

	return aDelta > bDelta;
}

static bool compareMoves(Move a, Move b) {
	if (a.isCapture() && b.isCapture()) {
		return compareCaptures(a, b);
	}
	if (a.isCapture()) {
		return true;
	}
	return false;
}

#define DO_KILLER_MOVES

void MovePicker::orderMoves(MoveList& ml, Move ttMove, int ply) {
	// First, place the hash move before all other moves
	if (ttMove != MOVE_INVALID) {
		int idx = ml.indexOf(ttMove);
		if (idx != -1) {
			std::swap(ml[0], ml[idx]);
		}
		else {
			// Specified ttMove was invalid
			ttMove = MOVE_INVALID;
		}
	}

	int nKillers = 0;
#ifdef DO_KILLER_MOVES
	if (ml.count() > (N_KILLER_MOVES + 1)) {
		for (int i = 0; i < N_KILLER_MOVES; ++i) {
			auto killer = m_Killers[i][ply];

			int idx = ml.indexOf(m_Killers[i][ply]);
			if (idx != -1) {
				std::swap(ml[1 + nKillers], ml[idx]);
				nKillers++;
			}
		}
	}
#endif
	
	std::sort(ml.begin() + (static_cast<size_t>(nKillers) + (ttMove != MOVE_INVALID ? 1 : 0)), ml.end(), compareMoves);
}

void MovePicker::orderMovesQuiesce(MoveList& ml, int ply) {
	
	std::sort(ml.begin(), ml.end(), compareCaptures);
}

int MovePicker::quiesce(Position& pos, int ply, int alpha, int beta) {
	if (pos.isDraw()) {
		return m_Eval.getDrawScore();
	}
	int standPat = m_Eval.evaluate(pos);

	if (standPat >= beta) {
		return beta;
	}

	if (standPat > alpha) {
		alpha = standPat;
	}

	MoveList moves;
	int moveCount;
	//if (!pos.isCheck()) {

	if (true) {
		moveCount = pos.getLegalMoves(moves, MoveFlags::Captures);
		orderMovesQuiesce(moves, ply);
	}

	Move bestMove = MOVE_INVALID;

	for (int i = 0; i < moveCount; ++i) {
		auto& move = moves[i];

		pos.makeMove(move, false, true);
		int score = -quiesce(pos, ply + 1, -beta, -alpha);
		pos.undoMove(move);

		if (score >= beta) {
			return beta;
		}

		if (score > alpha) {
			alpha = score;
		}
	}

	return alpha;
}

void MovePicker::storeKillerMove(Move move, int ply) {
	Move firstKiller = m_Killers[0][ply];

	if (firstKiller != move) {
		for (int i = 1; i < N_KILLER_MOVES; ++i) {
			int n = i;
			auto previous = m_Killers[n - 1][ply];
			m_Killers[n][ply] = previous;
		}

		m_Killers[0][ply] = move;
	}
}

std::tuple<Move, int> MovePicker::pickMoveAndScore(Position& pos, int depth, int ply, int alpha, int beta, bool us) {
	if (depth == 0) {
		int score = quiesce(pos, ply, alpha, beta);
		return std::make_tuple(MOVE_INVALID, score);
	}
	
	const int originalAlpha = alpha;
	int drawScore = us ? m_Eval.getDrawScore() : -m_Eval.getDrawScore();
	ui64 posKey = pos.getZobristKey();

	// We might have already found the best move for this position in 
	// a search with equal or higher depth. Look for it in the TT.
	TranspositionTable::Entry ttEntry = {};
	ttEntry.move = MOVE_INVALID;

	bool foundInTT = m_TT.tryGet(posKey, ttEntry);	
	if (foundInTT && ttEntry.depth >= depth) {
		if (ttEntry.type == TranspositionTable::EXACT) {
			return std::make_tuple(ttEntry.move, ttEntry.score);
		}
		else if (ttEntry.type == TranspositionTable::LOWERBOUND) {
			alpha = std::max(alpha, ttEntry.score);
		}
		else if (ttEntry.type == TranspositionTable::UPPERBOUND) {
			beta = std::min(beta, ttEntry.score);
		}

		if (alpha >= beta) {
			return std::make_tuple(ttEntry.move, ttEntry.score);
		}
	}

	// Check extension -- do not decrease depth when searching for moves
	// in positions in check.
	bool isCheck = pos.isCheck();
	if (isCheck) {		
		//depth++;
	}
	else {
		depth--;
	}

	MoveList moves;
	int moveCount = pos.getLegalMoves(moves);

	// If no legal moves, we're either in checkmate or stalemate
	if (moveCount == 0) {
		if (isCheck) {
			// We're being checkmated
			return std::make_tuple(MOVE_INVALID, -MATE_IN_ONE_SCORE);
		}
		// Stalemate draw
		return std::make_tuple(MOVE_INVALID, drawScore);
	}

	// Perform move ordering for better beta-cutoffs
	orderMoves(moves, ttEntry.move, ply);

	// After ordered, we can still make use of a move found in the transposition table by
	// placing it at the start of the move list.
	int bestMoveIdx = 0;

	// Finally, do the search
	for (int i = 0; i < moveCount; ++i) {
		auto move = moves[i];

		pos.makeMove(move, false, true);

		int score;
		Move _;

		if (pos.get50moveRuleCounter() <= 48 && !pos.getDrawList().contains(pos.getZobristKey())) {
			std::tie(_, score) = pickMoveAndScore(pos, depth, ply + 1, -beta, -alpha, !us);
			score = -score;
		}
		else {
			score = drawScore;
		}

		pos.undoMove(move);

		if (score >= beta) {
			alpha = beta;
			storeKillerMove(move, ply);
			break;
		}
		if (score > alpha) {
			alpha = score;
			bestMoveIdx = i;
		}
	}

	if (alpha > FORCED_MATE_THRESHOLD) {
		// Mate in a few moves, reduce one score
		alpha--;
	}

	auto bestMove = moves[bestMoveIdx];

	if (alpha <= originalAlpha) {
		ttEntry.type = TranspositionTable::UPPERBOUND;
	}
	else if (alpha >= beta) {
		ttEntry.type = TranspositionTable::LOWERBOUND;
	}
	else {
		ttEntry.type = TranspositionTable::EXACT;
	}

	ttEntry.depth = depth;
	ttEntry.move = bestMove;
	ttEntry.score = alpha;
	ttEntry.zobristKey = posKey;

	m_TT.maybeAdd(ttEntry);
	return std::make_tuple(bestMove, alpha);
}

#define DO_ITERATIVE_DEEPENING

std::tuple<Move, int> MovePicker::pickMove(const Position& pos, int depth) {
	if (depth > MAX_DEPTH) {
		return std::make_tuple(MOVE_INVALID, 0);
	}

	// Replicate the position
	Position newPos = pos;
	
	// Check if position is covered in our openings book.
	auto bookMove = m_OpBook.getMoveForPosition(pos);
	if (bookMove != MOVE_INVALID) {
		// Position is covered in our openings book. However,
		// we still want to calculate the score from now on.
		newPos.makeMove(bookMove);
		depth--;
	}

	Move move;
	int score;

	int materialCount = newPos.countTotalPointValue();

	if (materialCount <= 26) {
		//depth++;
	}
	if (materialCount <= 22) {
		depth++;
	}
	if (materialCount <= 12) {
		//depth++;
	}
	if (materialCount <= 7) {
		depth++;
	}

#ifdef DO_ITERATIVE_DEEPENING

	for (int i = std::max(depth - 2, 1); i <= depth; ++i) {
		std::tie(move, score) = pickMoveAndScore(newPos, i, 0, -HIGH_BETA, HIGH_BETA, true);
	}

	if (bookMove != MOVE_INVALID) {
		move = bookMove;
	}
	m_TT.clear();

	return std::make_tuple(move, score);

#else
	std::tie(move, score) = pickMoveAndScore(newPos, depth, 0, -HIGH_BETA, HIGH_BETA, true);

	if (bookMove != MOVE_INVALID) {
		move = bookMove;
	}
	m_TT.clear();

	return std::make_tuple(move, score);
#endif
}

}