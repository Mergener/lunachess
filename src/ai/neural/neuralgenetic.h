#ifndef LUNA_AI_NEURAL_NEURALGENETIC_H
#define LUNA_AI_NEURAL_NEURALGENETIC_H

#include <filesystem>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

#include "neuraleval.h"

namespace lunachess::ai::neural {

class Agent {
public:
    void mutate(int mutationRatePct);

    inline void randomize() {
        m_Eval.getNetwork().randomize(-1, 1);
    }

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);

    Agent();
    Agent(const Agent& a, const Agent& b);
    ~Agent() = default;

private:
    NeuralEvaluator m_Eval;
    std::string m_Name;
    int m_Score;
    int m_Gen;
};

class Generation {
public:


private:

};

class NeuralGeneticTraining {
public:
    void save(std::string_view path);

private:

};

} // lunachess::ai::neural

#endif // LUNA_AI_NEURAL_NEURALGENETIC_H