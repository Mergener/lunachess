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

struct GeneticTrainingSettings {
    int agentsPerGeneration = 16;
    int baseMutationRate = 10;
    int mutationRatePerGen = -1;
    int minMutationRate = 1;
    int matchesPerPairing = 2;
    TimeControl timeControl = TimeControl(50, 0, TC_MOVETIME);
};

}

#endif //NN_H