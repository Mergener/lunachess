#include "movepicker.h"

#include <algorithm>

#include "../core/position.h"

namespace lunachess::ai {

bool MovePicker::compareCaptures(Move a, Move b) const {
	int aDelta = a.getDestPiece().getPointValue() - a.getSourcePiece().getPointValue();
	int bDelta = b.getDestPiece().getPointValue() - b.getSourcePiece().getPointValue();

    if (aDelta > bDelta) {
        return true;
    }
    if (bDelta > aDelta) {
        return false;
    }

    // Deltas equal, return butterfly count
    int bfa = m_Butterflies[a.getSource()][a.getDest()];
    int bfb = m_Butterflies[b.getSource()][b.getDest()];

    return bfa > bfb;
}

bool MovePicker::compareMoves(const Move& a, const Move& b) const {
    LUNA_ASSERT(a != MOVE_INVALID && b != MOVE_INVALID, "Moves cannot be invalid");

	// Evaluate captures first
	if (a.isCapture() && b.isCapture()) {
		return compareCaptures(a, b);
	}
	if (a.isCapture()) {
		return true;
	}
    if (b.isCapture()) {
        return false;
    }

    // Return based on butterfly score
    int bfa = m_Butterflies[a.getSource()][a.getDest()];
    int bfb = m_Butterflies[b.getSource()][b.getDest()];

    if (bfa > bfb) {
        return true;
    }
    else if (bfb > bfa) {
        return false;
    }


	return false;
}

#define DO_KILLER_MOVES

void MovePicker::orderMoves(MoveList& ml, Move ttMove, int ply) {
	int start = 0;
#ifdef DO_KILLER_MOVES
	int nKillers = 0;
    if (ml.count() > (N_KILLER_MOVES + 1)) {
		for (int i = 0; i < N_KILLER_MOVES; ++i) {
			auto killer = m_Killers[i][ply];

			int idx = ml.indexOf(killer);
			if (idx != -1) {
				// Killer move is a legal move, count it.
				std::swap(ml[nKillers], ml[idx]);
				nKillers++;
			}
		}
	}
	start += nKillers;
#endif
    if (ttMove != MOVE_INVALID) {
        int idx = ml.indexOf(ttMove);
        if (idx != -1) {
            std::swap(ml[0], ml[idx]);
			start++;
        }
    }

    std::sort(ml.begin() + start, ml.end(),
              [this](Move a, Move b) { return compareMoves(a, b); });

}

void MovePicker::orderMovesQuiesce(MoveList& ml, int ply) {
	std::sort(ml.begin(), ml.end(),
              [this](Move a, Move b) { return compareCaptures(a, b); });
}

int MovePicker::quiesce(int ply, int alpha, int beta) {
	int standPat = m_Eval.evaluate(m_Position);

	if (standPat >= beta) {
		return beta;
	}

	if (standPat > alpha) {
		alpha = standPat;
	}

	MoveList moves;
	int moveCount = m_Position.getLegalMoves(moves, MoveFlags::Captures);
    orderMovesQuiesce(moves, ply);

	for (int i = 0; i < moveCount; ++i) {
		auto& move = moves[i];
		m_Butterflies[move.getSource()][move.getDest()] += 1;

        m_Position.makeMove(move, false, true);
		int score = -quiesce(ply + 1, -beta, -alpha);
        m_Position.undoMove(move);

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

bool MovePicker::canSearchNullMove(bool sideInCheck) const {
    if (sideInCheck) {
        return false;
    }

    constexpr int MIN_PIECE_COUNT = 2;
    int pieceCount = 0;

    // Do not perform null move heuristic if we only have king and pawns
    FOREACH_PIECE_TYPE(pt) {
        if (pt == PieceType::Pawn || pt == PieceType::King) {
            continue;
        }

        auto bb = m_Position.getPieceBitboard(Piece(m_Position.getSideToMove(), pt));
        pieceCount += bb.count();
    }
    if (pieceCount < MIN_PIECE_COUNT) {
        return false;
    }

    return true;
}

bool MovePicker::squareCanBeCapturedByPawn(Square s, Side side) const {
    LUNA_ASSERT(squares::isValid(s), "Square must be valid. (got " << (int)s << ")");
    Bitboard pawnBB = m_Position.getPieceBitboard(Piece(side, PieceType::Pawn));
    Bitboard pawnSquares = bitboards::getPawnAttacks(pawnBB, s, getOppositeSide(side)) & pawnBB;
    return pawnSquares == 0;
}

int MovePicker::searchInternal(int depth, int ply, int alpha, int beta, bool us, bool nullMove) {
    int drawScore = m_Eval.getDrawScore();

	if (depth == 0) {
		int score = quiesce(ply, alpha, beta);
		return score;
	}
	
	const int originalAlpha = alpha;
	ui64 posKey = m_Position.getZobristKey();

	// We might have already found the best move for this position in 
	// a pickMove with equal or higher depth. Look for it in the TT.
	TranspositionTable::Entry ttEntry = {};
	ttEntry.move = MOVE_INVALID;

	bool foundInTT = m_TT.tryGet(posKey, ttEntry);	
	if (foundInTT && ttEntry.depth >= depth) {
		if (ttEntry.type == TranspositionTable::EXACT) {
			return ttEntry.score;
		}
		else if (ttEntry.type == TranspositionTable::LOWERBOUND) {
			alpha = std::max(alpha, ttEntry.score);
		}
		else if (ttEntry.type == TranspositionTable::UPPERBOUND) {
			beta = std::min(beta, ttEntry.score);
		}

		if (alpha >= beta) {
			return ttEntry.score;
		}
	}

	// Check extension -- do not decrease depth when searching for moves
	// in positions in check.
	bool isCheck = m_Position.isCheck();

	if (isCheck) {		
		//depth++;
	}
	else {
		depth--;
	}

    // Null move pruning
/*
    if (!nullMove && canSearchNullMove(isCheck) && (depth - NULL_MOVE_SEARCH_DEPTH_REDUC) >= 1) {
        // We can perform null move pruning.
        int score;

        auto side = m_Position.getSideToMove();

        m_Position.setSideToMove(getOppositeSide(side));
        score = searchInternal(depth - NULL_MOVE_SEARCH_DEPTH_REDUC, ply + 1, -beta, -beta + 1, !us, true);
        score = -score;
        m_Position.setSideToMove(side);

        if (score >= beta) {
            // A cutoff was caused.
            return beta;
        }
    }*/

    MoveList moves;
	int moveCount = m_Position.getLegalMoves(moves);

	// If no legal moves, we're either in checkmate or stalemate
	if (moveCount == 0) {
		if (isCheck) {
			// We're being checkmated
			return -MATE_IN_ONE_SCORE + ply;
		}
		// Stalemate draw
		return drawScore;
	}

	// Perform move ordering for better beta-cutoffs
	orderMoves(moves, ttEntry.move, ply);

	// After ordered, we can still make use of a move found in the transposition table by
	// placing it at the start of the move list.
	int bestMoveIdx = 0;

	// Finally, do the pickMove
	for (int i = 0; i < moveCount; ++i) {
		auto move = moves[i];

        // Update butterfly table
        Square src = move.getSource();
        Square dest = move.getDest();
        m_Butterflies[src][dest] += 1;

		m_Position.makeMove(move, false, true);
        int score;
        if (m_Position.isDraw(1)) {
            score = -drawScore;
        }
        else {
			int d = depth;

            score = -searchInternal(d, ply + 1, -beta, -alpha, !us, false);
        }

		m_Position.undoMove(move);

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

	auto bestMove = moves[bestMoveIdx];

	if (alpha <= originalAlpha) {
		ttEntry.type = TranspositionTable::UPPERBOUND;
	}
	else if (alpha >= beta) {
		ttEntry.type = TranspositionTable::LOWERBOUND;
	}
	else {
		//std::cout << bestMoveIdx << "/" << moveCount << std::endl;
		ttEntry.type = TranspositionTable::EXACT;
	}

	ttEntry.depth = depth;
	ttEntry.move = bestMove;
	ttEntry.score = alpha;
	ttEntry.zobristKey = posKey;

	m_TT.maybeAdd(ttEntry);
	return alpha;
}

std::tuple<Move, int> MovePicker::pickMove(const Position& pos, int depth) {
    while (m_Lock); // Wait until current lock ends
    try {
        m_Lock = true;

        // Cannot search with depth higher than MAX_DEPTH
        if (depth > MAX_DEPTH) {
            m_Lock = false;
            return std::make_tuple(MOVE_INVALID, 0);
        }

        // Reset search values
        reset();

        // Replicate the position
        m_Position = pos;

        // Check if position is covered in our openings book
        auto bookMove = m_OpBook.getMoveForPosition(pos);
        if (bookMove != MOVE_INVALID) {
            // Position is covered in our openings book. However,
            // we still want to calculate the score from now on
            m_Position.makeMove(bookMove);
            depth--;
        }

        // Perform search with iterative deepening
        MoveList moves;
        m_Position.getLegalMoves(moves);
        if (moves.count() == 0) {
            // No legal moves
            int score = m_Position.isCheck() ? -MATE_IN_ONE_SCORE : 0;
            return std::make_tuple(MOVE_INVALID, score);
        }
        Move bestMove = moves[0];
        int bestScore;

		int drawScore = m_Eval.getDrawScore();

        // Perform search with iterative deepening
        for (int i = std::max(depth - 2, 2); i <= depth; ++i) {
            bestScore = -HIGH_BETA;
	        orderMoves(moves, bestMove, 0);
            for (int j = 0; j < moves.count(); ++j) {
	            Move move = moves[j];
                m_Position.makeMove(move, false, true);

                int score;

				int d = i;
	            if (m_Position.isDraw(2)) {
		            score = -drawScore;
	            }
				else {
		            score = -searchInternal(d, 1, -HIGH_BETA, -bestScore, false, false);
	            }

                m_Position.undoMove(move);

                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }
            }
        }

        if (bookMove != MOVE_INVALID) {
            bestMove = bookMove;
        }
        m_Lock = false;

        return std::make_tuple(bestMove, bestScore);
    }
    catch (const std::exception& ex) {
        m_Lock = false;
        return std::make_tuple(MOVE_INVALID, 0);
    }
}

void MovePicker::reset() {
    m_TT.clear();
    m_TT.reserve(16384);
    std::memset(&m_Butterflies[0][0], 0, 64 * 64 * sizeof(int));
}

}