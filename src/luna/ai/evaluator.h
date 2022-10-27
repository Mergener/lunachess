#ifndef LUNA_AI_EVALUATOR_H
#define LUNA_AI_EVALUATOR_H

#include "../position.h"

namespace lunachess::ai {

class Evaluator {
public:
    virtual int getDrawScore() const = 0;
    virtual int evaluate(const Position& pos) const = 0;
    inline virtual int evaluateShallow(const Position& pos) const {
        return evaluate(pos);
    }
    inline virtual void preparePosition(const Position& pos) {}
    inline virtual void onMakeMove(Move move) const {}
    inline virtual void onUndoMove(Move move) const {}
    inline virtual void onMakeNullMove() const {}
    inline virtual void onUndoNullMove() const {}

    virtual ~Evaluator() = default;
};

}

#endif // LUNA_AI_EVALUATOR_H