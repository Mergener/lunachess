#include "quiescevaluator.h"

#include "search.h"

namespace lunachess::ai {

int QuiesceEvaluator::evaluate() const {
    return quiesce(5, -HIGH_BETA, HIGH_BETA);
}

int QuiesceEvaluator::quiesce(int depth, int alpha, int beta) const {
    const Position& pos = m_Eval->getPosition();
    int standPat = m_Eval->evaluate();

    if (depth <= 0) {
        return standPat;
    }
    if (standPat >= beta) {
        // Fail high
        return beta;
    }

    if (standPat > alpha) {
        // Before we search, if the standPat is higher than alpha then it means
        // that not performing a noisy move is best (so far).
        alpha = standPat;
    }

    MoveList moves;
    int moveCount = m_MvFactory.generateNoisyMoves(moves, pos, 0);

    for (int i = 0; i < moveCount; ++i) {
        Move move = moves[i];
        if (move.getType() == MT_SIMPLE_CAPTURE &&
            !posutils::hasGoodSEE(pos, move)) {
            // The result of the exchange series will always result in
            // material loss after this capture, prune it.
            continue;
        }

        m_Eval->makeMove(move);
        int score = -quiesce(depth - 1, -beta, -alpha);
        m_Eval->undoMove();

        if (score >= beta) {
            // Fail high
            return beta;
        }

        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}

}