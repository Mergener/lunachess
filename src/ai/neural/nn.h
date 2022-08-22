#ifndef NN_H
#define NN_H

#include "../evaluator.h"
#include "../../position.h"

namespace lunachess::ai::neural {

class NeuralNetwork;

class NeuralEvaluator : public Evaluator {
public:
    NeuralEvaluator();
    ~NeuralEvaluator();

    inline int getDrawScore() const override { return 0; }
    int evaluate(const Position& pos) const override;

private:
    NeuralNetwork* m_Network;


};

}

#endif //NN_H