#ifndef LUNA_AI_NEURAL_NEURALGENETIC_H
#define LUNA_AI_NEURAL_NEURALGENETIC_H

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>
#include <optional>

#include "neuraleval.h"
#include "../../position.h"

namespace lunachess::ai::neural {

struct Agent {
    std::string id;
    int generationNumber = 0;
    std::shared_ptr<NeuralEvaluator::NN> network = nullptr;
};

struct TrainingGameMoveData {
    Move move;
    int score;
};

struct TrainingGame {
    std::string id;
    std::array<Agent*, 2> agents; // First plays black, second plays white
    std::vector<TrainingGameMoveData> moves;
    ChessResult resultForWhite = RES_UNFINISHED;
};

struct Generation {
    int number = 0;
    std::vector<Agent*> agents;
    std::vector<TrainingGame*> games;
    std::unordered_map<std::string, int> agentFitness;
};

struct GeneticTrainingSettings {
    std::filesystem::path baseSavePath;

    int agentsPerGeneration = 12;
    int cullingRate = 8;
    float baseMutationRate = 25;
    float mutationRatePerGen = -0.21;
    float minMutationRate = 5;
    int matchesPerPairing = 1;
    TimeControl timeControl = TimeControl(0, 0, TC_INFINITE);
    int maxDepth = 1;

    std::function<void()> onIterationFinish = nullptr;
};

class GeneticTraining {
public:
    inline const Generation& getCurrentGeneration() const {
        return m_CurrGeneration;
    }

    inline const GeneticTrainingSettings& getSettings() const { return m_Settings; }
    inline const Agent& getAgent(const std::string& id) const { return *m_Agents.at(id); }

    inline bool isRunning() const { return m_ItLeft != 0; }

    inline void updateSettings(const GeneticTrainingSettings& settings) {
        m_Settings = settings;
    }

    void run(int iterations = -1);
    void stop();

private:
    std::unordered_map<std::string, std::unique_ptr<Agent>> m_Agents;
    std::unordered_map<std::string, std::unique_ptr<TrainingGame>> m_Games;
    Generation m_CurrGeneration;
    GeneticTrainingSettings m_Settings;
    int m_ItLeft = 0;
    float m_CurrMutRate = 0;

    using GamePairings = std::vector<std::pair<Agent*, Agent*>>;
    GamePairings generatePairings();

    Agent* createEmptyAgent(std::string_view id);
    Agent* createRandomAgent();

    void removeAgent(const std::string& agentId);
    inline void removeAgent(const Agent& a) {
        removeAgent(a.id);
    }

    void playGames(const GamePairings& pairings);
    void cull();
    void reproduceAgents();
    void save() const;
};

void to_json(nlohmann::json&, const Agent&);
void to_json(nlohmann::json&, const Generation&);
void from_json(const nlohmann::json&, Agent&);
void from_json(const nlohmann::json&, Generation&);

} // lunachess::ai::neural

#endif // LUNA_AI_NEURAL_NEURALGENETIC_H