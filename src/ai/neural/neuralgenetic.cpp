#include "neuralgenetic.h"

#include <algorithm>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <iostream>
#include <fstream>

#include "../search.h"

namespace lunachess::ai::neural {

using NN = NeuralEvaluator::NN;

namespace fs = std::filesystem;

//
// Utils
//

static std::string randomId() {
    char ret[64];

    ui32 a = utils::random(static_cast<ui32>(0), UINT32_MAX);
    ui32 b = utils::random(static_cast<ui32>(0), UINT32_MAX);
    ui32 c = utils::random(static_cast<ui32>(0), UINT32_MAX);
    ui32 d = utils::random(static_cast<ui32>(0), UINT32_MAX);

    int len = sprintf(ret, "%04x-%04x-%04x-%04x", a, b, c, d);
    ret[len] = '\0';

    return ret;
}

//
// GeneticTraining
//


void GeneticTraining::save() const {
    auto basePath = m_Settings.baseSavePath;

    // Create directories
    fs::create_directories(basePath);

    fs::create_directory(basePath / "generations");
    fs::create_directory(basePath / "agents");
    fs::create_directory(basePath / "games");

    // Save current generation
    std::fstream genStream(basePath / "generations" / utils::toString(m_CurrGeneration.number));
    genStream << nlohmann::json(m_CurrGeneration) << std::endl;
    genStream.close();

    // Save all agents
}

void GeneticTraining::stop() {
    if (!isRunning()) {
        return;
    }

    m_ItLeft = 0;
}

void GeneticTraining::run(int iterations) {
    if (iterations == 0) {
        // Starting training with no iterations
        return;
    }

    if (isRunning()) {
        // Cannot start training, it is already ongoing.
        return;
    }

    m_ItLeft = iterations;

    // Do we have the number of agents we want to have?
    while (m_CurrGeneration.otherAgents.size() < m_Settings.agentsPerGeneration) {
        auto agent = createRandomAgent();
        m_CurrGeneration.otherAgents.push_back(agent);
    }

    while (m_ItLeft != 0) {
        if (m_ItLeft > 0) {
            // Only count iterations if they are positive.
            // Negative iterations numbers mean that the caller wants the training
            // to go on indefinitely.
            m_ItLeft--;
        }

        auto pairings = generatePairings();
        playGames(pairings);
        save();
        cull();
        reproduceAgents();
    }
}

Agent* GeneticTraining::createRandomAgent() {
    auto agent = std::make_unique<Agent>();
    Agent* ret = agent.get();

    m_Agents[agent->id] = std::move(agent);

    return ret;
}

/**
 * @brief Plays a game between to agents.
 * 
 * @param white The agent to play as white
 * @param black The agent to play as black
 * @return A TrainingGame object with the game's data.
 */
static std::unique_ptr<TrainingGame> playAgentGame(Agent* white, Agent* black, const TimeControl& tc) {
    auto ret = std::make_unique<TrainingGame>();
    Position pos = Position::getInitialPosition();

    // Fill in basic game data
    ret->id = randomId();
    ret->agents[0] = white;
    ret->agents[1] = black;

    std::cout << "Game " << ret->id << " (" << white->id << " vs "
        << black->id << ") is STARTING." << std::endl;

    // Generate searchers
    AlphaBetaSearcher searchers[] = {
        AlphaBetaSearcher(std::make_shared<NeuralEvaluator>(white->network)),
        AlphaBetaSearcher(std::make_shared<NeuralEvaluator>(black->network))
    };

    // Configure time controls
    SearchSettings settings;
    settings.ourTimeControl = tc;
    settings.theirTimeControl = tc;

    Color curr = pos.getColorToMove();
    bool flagged = false;
    ChessResult gameRes;

    // Play the game until it ends, either by clock or board
    while ((gameRes = pos.getResult(CL_WHITE, !flagged)) != RES_UNFINISHED) {
        SearchResults res = searchers[curr].search(pos, settings);

        if (res.getSearchTime() > settings.ourTimeControl.time) {
            // Player flagged
            flagged = true;
            continue;
        }

        // Tick time for Fischer based time controls
        if (tc.mode == TC_FISCHER) {
            settings.ourTimeControl.time -= int(res.getSearchTime());
            settings.ourTimeControl.time += tc.increment;
            std::swap(settings.ourTimeControl, settings.theirTimeControl);
        }

        Move move = res.bestMove;
        pos.makeMove(move);
        ret->moves.push_back(move);
        curr = pos.getColorToMove();
    }

    // Game is over, fill in result, log and return
    ret->resultForWhite = gameRes;

    std::cout << "Game " << ret->id << " finished. Result: "
        << (isDraw(gameRes) ? "DRAW"
          : isWin(gameRes) ? "WHITE WINS" : "BLACK WINS")
        << std::endl;

    return ret;
}

GeneticTraining::GamePairings GeneticTraining::generatePairings() {
    GamePairings pairings;

    const Generation& gen = getCurrentGeneration();
    Agent* targetAgent = gen.targetAgent;
    auto& agentsToPlay = gen.otherAgents;

    for (int i = 0; i < agentsToPlay.size() - 1; ++i) {
        for (int j = 0; j < m_Settings.matchesPerPairing; ++j) {
            pairings.emplace_back(agentsToPlay[i], targetAgent);
            pairings.emplace_back(targetAgent, agentsToPlay[i]);
        }
    }

    return pairings;
}

void GeneticTraining::playGames(const GamePairings& pairings) {
    constexpr int WIN_BONUS = 10000;

    for (const auto& pair: pairings) {
        Agent* white = pair.first;
        Agent* black = pair.second;
        auto game = playAgentGame(white, black, m_Settings.timeControl);

        // Do not mistake moves for plies!
        int moveCount = game->moves.size() / 2;
        
        // Update fitness scores
        if (isWin(game->resultForWhite)) {
            if (white != m_CurrGeneration.targetAgent) {
                m_CurrGeneration.agentFitness[white->id] += WIN_BONUS - moveCount;
            }
            
            if (black != m_CurrGeneration.targetAgent) {
                m_CurrGeneration.agentFitness[black->id] += moveCount - WIN_BONUS;
            }
        }
        else if (isLoss(game->resultForWhite)) {
            if (white != m_CurrGeneration.targetAgent) {
                m_CurrGeneration.agentFitness[white->id] += moveCount - WIN_BONUS;
            }
            
            if (black != m_CurrGeneration.targetAgent) {
                m_CurrGeneration.agentFitness[black->id] += WIN_BONUS - moveCount;
            }
        } // Else, game ended in a draw

        m_Games[game->id] = std::move(game);
    }
}

void GeneticTraining::cull() {
    // Target agent becomes 'part' of the other agents.
    // It will be culled if agents managed to win against it.
    m_CurrGeneration.otherAgents.push_back(m_CurrGeneration.targetAgent);

    // Sort agents from fittest to less fit
    std::sort(m_CurrGeneration.otherAgents.begin(), m_CurrGeneration.otherAgents.end(),
              [this](auto a, auto b) {
        return m_CurrGeneration.agentFitness[a->id] > m_CurrGeneration.agentFitness[b->id];
    });

    // Remove unfit agents
    for (auto it = m_CurrGeneration.otherAgents.rbegin();
         m_CurrGeneration.otherAgents.size() > m_Settings.agentsPerGeneration;
         ++it) {
        // Remove the agent from both the generation's vector and the global dictionary
        Agent* agent = *it;
        m_Agents.erase(agent->id);
        m_CurrGeneration.agentFitness.erase(agent->id);
        m_CurrGeneration.otherAgents.erase(std::next(it).base());
    }

    // Erase stored games and agent fitnesses
    m_CurrGeneration.games.clear();

    // Zero out agent fitnesses
    for (auto& fitness: m_CurrGeneration.agentFitness) {
        fitness.second = 0;
    }
}

static std::unique_ptr<Agent> crossoverAgents(int genNumber, const Agent& a, const Agent& b) {
    auto ret = std::make_unique<Agent>();

    ret->id = randomId();
    ret->network = std::make_shared<NN>(*a.network, *b.network);
    ret->generationNumber = genNumber;

    return ret;
}

static void mutateAgent(Agent& a, int mutationRatePct) {
    a.network->mutate(mutationRatePct);
}

void GeneticTraining::reproduceAgents() {
    m_CurrGeneration.number++;

    // Perform crossovers
    Agent* parentA = m_CurrGeneration.otherAgents[0];
    Agent* parentB = m_CurrGeneration.otherAgents[1];

    // Fittest agent becomes the new target agent
    m_CurrGeneration.otherAgents.erase(m_CurrGeneration.otherAgents.begin());
    m_CurrGeneration.targetAgent = parentA;
    while (m_CurrGeneration.otherAgents.size() < m_Settings.agentsPerGeneration) {
        auto agent = crossoverAgents(m_CurrGeneration.number, *parentA, *parentB);
        mutateAgent(*agent, m_CurrMutRate);

        m_Agents[agent->id] = std::move(agent);
    }

    m_CurrMutRate += m_Settings.mutationRatePerGen;
}

// JSON Serialization methods

// The following macro calls will create 'toJson' and 'fromJson' methods automatically
// for the neural networks.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NN::TFirstHidden, weights, biases);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NN::THidden, weights, biases);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NN::TOut, weights, biases);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NN, firstHidden, outputLayer, inBetweenLayers);
//

void to_json(nlohmann::json& j, const Agent& a) {
    j["id"] = a.id;
    j["gen"] = a.generationNumber;
    j["network"] = *a.network;
}

void from_json(const nlohmann::json& j, Agent& a) {
    a.id = j.get<std::string>();
    a.generationNumber = j.get<int>();
    a.network = std::make_shared<NN>(j.get<NN>());
}

void to_json(nlohmann::json& j, const Generation& gen) {
    j["number"] = gen.number;
    j["targetAgent"] = gen.targetAgent->id;

    // Fetch agent ids
    std::unordered_map<std::string, int> agentsAndFitness;
    for (const auto& ag: gen.otherAgents) {
        agentsAndFitness[ag->id] = gen.agentFitness.at(ag->id);
    }
    j["agentsAndFitness"] = agentsAndFitness;

    // Fetch game ids
    std::vector<std::string> gameIds;
    for (const auto& g: gen.games) {
        gameIds.push_back(g->id);
    }
    j["games"] = gameIds;
}

void from_json(const nlohmann::json&, Generation&) {
    // TO-DO
}


} // lunachess::ai::neural