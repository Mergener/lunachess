#include "quiescevaluator.h"

#include "search.h"

namespace lunachess::ai {

int QuiesceEvaluator::evaluate(const Position &pos) const {
    Position repl = pos;
    return quiesce(repl, 5, -HIGH_BETA, HIGH_BETA);
}

int QuiesceEvaluator::quiesce(Position &pos, int depth, int alpha, int beta) const {
    int standPat = m_Eval->evaluate(pos);

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

        pos.makeMove(move);
        int score = -quiesce(pos, depth - 1, -beta, -alpha);
        pos.undoMove();

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