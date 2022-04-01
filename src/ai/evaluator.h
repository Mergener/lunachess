#ifndef LUNA_AI_EVALUATOR_H
#define LUNA_AI_EVALUATOR_H

#include "../position.h"

namespace lunachess::ai {

class Evaluator {
public:
    virtual int getDrawScore() const = 0;
    virtual int evaluate(const Position& pos) const = 0;

    virtual ~Evaluator() = default;
};

}

#endif // LUNA_AI_EVALUATOR_H