#ifndef LUNACHESS_AI_QUIESCEVALUATOR_H
#define LUNACHESS_AI_QUIESCEVALUATOR_H

#include <memory>

#include "classiceval/classicevaluator.h"
#include "aimovefactory.h"

namespace lunachess::ai {

class QuiesceEvaluator : public Evaluator {
public:
    int evaluate(const Position& pos) const override;
    inline int getDrawScore() const override { return 0; }

    inline QuiesceEvaluator()
        : m_Eval(std::make_shared<ClassicEvaluator>()){

    }

    inline QuiesceEvaluator(std::shared_ptr<Evaluator> eval)
        : m_Eval(eval) {}

private:
    std::shared_ptr<Evaluator> m_Eval;
    AIMoveFactory m_MvFactory;

    int quiesce(Position& pos, int depth, int alpha, int beta) const;
};

}

#endif // LUNACHESS_AI_QUIESCEVALUATOR_H
