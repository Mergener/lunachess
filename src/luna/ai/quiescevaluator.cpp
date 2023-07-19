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


    MoveCursor<true> moveCursor;
    Move move;
    while ((move = moveCursor.next(pos, m_MvOrderData, 0))) {
        if (move.getType() == MT_SIMPLE_CAPTURE &&
            !staticanalysis::hasGoodSEE(pos, move)) {
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