#ifndef LUNA_AI_NEURAL_GENETIC_H
#define LUNA_AI_NEURAL_GENETIC_H

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "neuraleval.h"

namespace lunachess::ai::neural {

struct Agent {
    std::string id;
    std::shared_ptr<EvalNN> nn = nullptr;
    ui64 avgLoss = 0;
    bool simulated = false;
};

struct Generation {
    int number;
    std::vector<Agent*> agents;

    inline Generation(int number = 0) noexcept
        : number(number) {}
};

struct GeneticTrainingSettings {
    std::filesystem::path savePath;
    std::filesystem::path datasetPath;
    int nDataFiles = 1000;
    float initialMutationRate = 80;
    float mutationRatePerGen = -0.5;
    float minMutationRate = 3;
    float maxMutationRate = 100;
    int mutDeltaWeight = 2;
    int mutDeltaBias = 2;
    int mutWeightPctChange = 6;
    int mutBiasPctChange = 6;

    int agentsPerGen = 24;
    int cutPoint = 16;
};

class GeneticTraining {
public:
    [[noreturn]] void run();

    inline GeneticTraining(GeneticTrainingSettings settings) noexcept
        : m_Settings(settings) {}

private:
    GeneticTrainingSettings m_Settings;
    std::unordered_map<std::string, std::unique_ptr<Agent>> m_Agents;
    Generation m_CurrGen;

    float m_CurrMutRate = 0;

    struct ReferenceData {
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReferenceData, fen, eval);
        std::string fen;
        int eval;
    };
    std::vector<ReferenceData> m_Dataset;

    void install();
    void loadDataset();
    void createRandomAgents(int n);
    void simulate();
    void selectSome();
    void reproduce();
    void save();

    void simulateAgent(Agent& a);
    void crossoverAgents(Agent& a, Agent& b);
    void mutateAgent(Agent& a);
};

} // lunachess::ai::neural

#endif // LUNA_AI_NEURAL_GENETIC_H