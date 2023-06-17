#ifndef LUNACHESS_AI_QUIESCEVALUATOR_H
#define LUNACHESS_AI_QUIESCEVALUATOR_H

#include <memory>

#include "classiceval/classicevaluator.h"
#include "aimovefactory.h"

namespace lunachess::ai {

class QuiesceEvaluator : public Evaluator {
public:
    int evaluate() const;
    inline int getDrawScore() const override { return 0; }

    inline QuiesceEvaluator(Evaluator* eval)
        : m_Eval(eval) {}

protected:
    inline void onSetPosition(const Position& pos) override {
        m_Eval->setPosition(pos);
    }

    inline void onMakeMove(Move move) override {
        m_Eval->makeMove(move);
    }

    inline void onUndoMove(Move move) override {
        m_Eval->undoMove();
    }

    inline void onMakeNullMove() override {
        m_Eval->makeNullMove();
    }

    inline void onUndoNullMove() override {
        m_Eval->undoNullMove();
    }

private:
    Evaluator* m_Eval;
    AIMoveFactory m_MvFactory;

    int quiesce(int depth, int alpha, int beta) const;
};

}

#endif // LUNACHESS_AI_QUIESCEVALUATOR_H
