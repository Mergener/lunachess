#ifndef LUNACHESS_AI_QUIESCEVALUATOR_H
#define LUNACHESS_AI_QUIESCEVALUATOR_H

#include "classicevaluator.h"
#include "aimovefactory.h"

namespace lunachess::ai {

class QuiesceEvaluator : public Evaluator {
public:
    int evaluate(const Position& pos) const override;
    inline int getDrawScore() const override { return 0; }

private:
    ClassicEvaluator m_Eval;
    AIMoveFactory m_MvFactory;

    int quiesce(Position& pos, int depth, int alpha, int beta) const;
};

}

#endif // LUNACHESS_AI_QUIESCEVALUATOR_H
