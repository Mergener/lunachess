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

    if (!squares::isValid(a.getSource())) {
        std::cout << (int)a.getSource() << "Invalid!\n";
    }
    if (!squares::isValid(a.getDest())) {
        std::cout << (int)a.getDest() << "Invalid!\n";
    }
    if (!squares::isValid(b.getSource())) {
        std::cout << (int)b.getSource() << "Invalid!\n";
    }
    if (!squares::isValid(b.getDest())) {
        std::cout << (int)b.getDest() << "Invalid!\n";
    }

    // Deltas equal, return butterfly count
    int bfa = 0;
    int bfb = 0;
    //int bfa = m_Butterflies[a.getSource()][a.getDest()];
    //int bfb = m_Butterflies[b.getSource()][b.getDest()];

    return bfa > bfb;
}

bool MovePicker::compareMoves(Move a, Move b) const {
	if (a.isCapture() && b.isCapture()) {
		return compareCaptures(a, b);
	}
	if (a.isCapture()) {
		return true;
	}

    if (!squares::isValid(a.getSource())) {
        std::cout << (int)a.getSource() << "Invalid!\n";
    }
    if (!squares::isValid(a.getDest())) {
        std::cout << (int)a.getDest() << "Invalid!\n";
    }
    if (!squares::isValid(b.getSource())) {
        std::cout << (int)b.getSource() << "Invalid!\n";
    }
    if (!squares::isValid(b.getDest())) {
        std::cout << (int)b.getDest() << "Invalid!\n";
    }

    // Return based on butterfly score
    int bfa = 0;
    int bfb = 0;
    //int bfa = m_Butterflies[a.getSource()][a.getDest()];
    //int bfb = m_Butterflies[b.getSource()][b.getDest()];

    return bfa > bfb;
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

			int idx = ml.indexOf(killer);
			if (idx != -1) {
				std::swap(ml[1 + nKillers], ml[idx]);
				nKillers++;
			}
		}
	}
#endif
	
	std::sort(ml.begin() + (static_cast<size_t>(nKillers) + (ttMove != MOVE_INVALID ? 1 : 0)), ml.end(),
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

int MovePicker::searchInternal(int depth, int ply, int alpha, int beta, bool us, bool nullMove) {
	if (depth == 0) {
		int score = quiesce(ply, alpha, beta);
		return score;
	}
	
	const int originalAlpha = alpha;
	int drawScore = m_Eval.getDrawScore();
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
    }


    MoveList moves;
	int moveCount = m_Position.getLegalMoves(moves);

	// If no legal moves, we're either in checkmate or stalemate
	if (moveCount == 0) {
		if (isCheck) {
			// We're being checkmated
			return -MATE_IN_ONE_SCORE;
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
        //m_Butterflies[src][dest] += 1;

		m_Position.makeMove(move, false, true);

		int score;

		if (m_Position.get50moveRuleCounter() <= 48 && !m_Position.getDrawList().contains(m_Position.getZobristKey())) {
			score = -searchInternal(depth, ply + 1, -beta, -alpha, !us);
		}
		else {
			score =  us ? drawScore : -drawScore;
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
	return alpha;
}

#define DO_ITERATIVE_DEEPENING

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

        // Increment depth if material is low enough
        int materialCount = m_Position.countTotalPointValue();
        if (materialCount <= 22) {
            depth++;
        }
        if (materialCount <= 7) {
            depth++;
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
        int bestScore = -HIGH_BETA;

        for (int i = std::max(depth - 2, 2); i <= depth; ++i) {
            orderMoves(moves, bestMove, 0);
            for (const auto& move : moves) {
                m_Position.makeMove(move);
                int score;
                if (m_Position.get50moveRuleCounter() <= 48 && !m_Position.getDrawList().contains(m_Position.getZobristKey())) {
                    score = -searchInternal(i - 1, 1, -HIGH_BETA, -bestScore, false);
                }
                else {
                    score = -m_Eval.getDrawScore();
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
    //std::memset(&m_Butterflies[0][0], 0, 64 * 64 * sizeof(int));
}

}