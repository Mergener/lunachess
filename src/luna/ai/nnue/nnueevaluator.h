#ifndef LUNA_AI_NNUEEVALUATOR_H
#define LUNA_AI_NNUEEVALUATOR_H

#include <memory>

#include "../evaluator.h"
#include "nnue.h"

namespace lunachess::ai {

class NNUEEvaluator : public Evaluator {
public:
    i32 evaluate() const override;

    inline i32 getDrawScore() const override {
        return 0;
    }

    inline NNUEEvaluator(std::shared_ptr<nnue::NNUE> net) {

    }

private:
    std::shared_ptr<nnue::NNUE> m_NNUE;
    nnue::Accumulator m_Accum;
};

}

#endif // LUNA_AI_NNUEEVALUATOR_H
