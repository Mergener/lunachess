#ifndef LUNA_EVALUATOR_H
#define LUNA_EVALUATOR_H

#include "../core/position.h"

namespace lunachess::ai {

class Evaluator {
public:
	virtual int evaluate(const Position& pos) const = 0;
	virtual int getDrawScore() const = 0;

	virtual ~Evaluator() = default;
};

}

#endif // LUNA_EVALUATOR_H