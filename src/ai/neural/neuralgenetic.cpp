#include "neuralgenetic.h"

#include <algorithm>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <iostream>
#include <fstream>

#include "../search.h"
#include "../quiescevaluator.h"

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

static void breakpoint(std::string msg) {
    std::cout << msg << std::endl;
    std::cin.get();
}

static void saveGameToStream(std::ostream& stream, const TrainingGame& game) {
    stream << "[White \"Agent " << game.agents[0]->id << "\"]" << std::endl;
    stream << "[Black \"Agent " << game.agents[1]->id << "\"]" << std::endl;

    stream << "[Result \"";
    if (isWin(game.resultForWhite)) {
        stream << "1-0";
    } else if (isLoss(game.resultForWhite)) {
        stream << "0-1";
    } else {
        stream << "1/2-1/2";
    }
    stream << "\"]" << std::endl;

    static constexpr int PLIES_PER_LINE = 2;
    for (int i = 0; i < game.moves.size(); ++i) {
        if (i >= PLIES_PER_LINE && (i % PLIES_PER_LINE == 0)) {
            stream << std::endl;
        }
        if (i % 2 == 0) {
            stream << (i / 2) + 1 << ". ";
        }
        const auto& data = game.moves[i];
        Move move = data.move;
        int score = data.score;

        stream << move << " { " << score << " cp } ";
    }
}

void GeneticTraining::save() const {
    auto basePath = m_Settings.baseSavePath;
    std::cout << "Saving neural genetic training at '" << basePath << "'..." << std::endl;

    // Create directories
    std::cout << "Creating directories..." << std::endl;
    std::cout << fs::absolute(basePath) << std::endl;

    fs::create_directories(basePath);

    fs::create_directory(basePath / "generations");
    fs::create_directory(basePath / "agents");

    // Save current generation
    std::string thisGenName = utils::toString(m_CurrGeneration.number);
    fs::path thisGenDir =  basePath / "generations" / thisGenName;
    fs::create_directory(thisGenDir);

    std::ofstream genStream(thisGenDir / "data.json");
    genStream.exceptions( std::ofstream::failbit | std::ofstream::badbit );

    genStream.exceptions( std::ofstream::failbit | std::ofstream::badbit );
    auto genJson = nlohmann::json(m_CurrGeneration);
    genStream << genJson << std::endl;

    genStream.close();

    // Save all games of the generation
    std::cout << "Saving games..." << std::endl;
    for (const auto& g: m_Games) {
        const auto& game = g.second;

        std::cout << "Saving game " << game->id << "..." << std::endl;

        std::ofstream gameStream(thisGenDir / (game->id + ".pgn"));
        gameStream.exceptions( std::ofstream::failbit | std::ofstream::badbit );

        saveGameToStream(gameStream, *game);

        gameStream.close();
    }

    // Save all agents
    std::cout << "Saving agents..." << std::endl;
    for (const auto& agent: m_CurrGeneration.agents) {
        std::cout << "Saving agent " << agent->id << "..." << std::endl;
        std::ofstream agentStream(basePath / "agents" / (agent->id + ".json"));
        agentStream.exceptions( std::ofstream::failbit | std::ofstream::badbit );
        agentStream << nlohmann::json(*agent) << std::endl;
        agentStream.close();
    }

    std::cout << "Genetic training saved successfully." << std::endl;
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
    while (m_CurrGeneration.agents.size() < m_Settings.agentsPerGeneration) {
        auto agent = createRandomAgent();
        m_CurrGeneration.agents.push_back(agent);
    }

    m_CurrMutRate = m_Settings.baseMutationRate;

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

        std::cout << "Genetic training iteration finished.";
        if (m_ItLeft > 0) {
            std::cout << m_ItLeft << " to go.";
        }
        std::cout << std::endl;
    }
}

Agent* GeneticTraining::createEmptyAgent(std::string_view id) {
    auto agent = std::make_unique<Agent>();
    agent->id = std::string(id);

    Agent* ret = agent.get();

    m_CurrGeneration.agentFitness[agent->id] = 0;
    m_Agents[agent->id] = std::move(agent);

    return ret;
}

Agent* GeneticTraining::createRandomAgent() {
    auto agent = std::make_unique<Agent>();
    agent->id = randomId();
    agent->network = std::make_shared<NN>();
    agent->network->randomize();
    agent->generationNumber = m_CurrGeneration.number;

    Agent* ret = agent.get();

    m_CurrGeneration.agentFitness[agent->id] = 0;
    m_Agents[agent->id] = std::move(agent);

    return ret;
}

void GeneticTraining::removeAgent(const std::string& id) {
    Agent* a = m_Agents.at(id).get();
    m_CurrGeneration.agentFitness.erase(id);
    auto it = std::find(m_CurrGeneration.agents.begin(), m_CurrGeneration.agents.end(), a);
    if (it != m_CurrGeneration.agents.end()) {
        m_CurrGeneration.agents.erase(it);
    }
}

static TrainingGameMoveData agentPickMove(const Evaluator& eval, Position& pos) {
    TrainingGameMoveData ret;

    MoveList ml;
    movegen::generate(pos, ml);

    Color us = pos.getColorToMove();

    Move bestMove = MOVE_INVALID;
    int bestScore = INT_MIN;
    for (Move m: ml) {
        pos.makeMove(m);

        ChessResult result = pos.getResult(us);
        if (isWin(result)) {
            bestMove = m;
            bestScore = MATE_SCORE;
            pos.undoMove();
            break;
        }

        int score;

        if (isLoss(result)) {
            score = -MATE_SCORE;
        }
        else if (!pos.isRepetitionDraw(1)) {
            score = -eval.evaluate(pos);
        }
        else {
            score = eval.getDrawScore();
        }

        if (score > bestScore) {
            bestMove = m;
            bestScore = score;
        }

        pos.undoMove();
    }

    ret.move = bestMove;
    ret.score = us == CL_WHITE ? bestScore : -bestScore;

    return ret;
}

/**
 * @brief Plays a game between to agents.
 * 
 * @param white The agent to play as white
 * @param black The agent to play as black
 * @return A TrainingGame object with the game's data.
 */
static std::unique_ptr<TrainingGame> playAgentGame(Agent* white, Agent* black, const TimeControl& tc, int maxDepth) {
    auto ret = std::make_unique<TrainingGame>();
    Position pos = Position::getInitialPosition();

    // Fill in basic game data
    ret->id = randomId();
    ret->agents[0] = white;
    ret->agents[1] = black;

    std::unique_ptr<Evaluator> evaluators[2];
    if (white->network != nullptr) {
        evaluators[CL_WHITE] = std::make_unique<NeuralEvaluator>(white->network);
    }
    else {
        evaluators[CL_WHITE] = std::make_unique<QuiesceEvaluator>();
    }
    if (black->network != nullptr) {
        evaluators[CL_BLACK] = std::make_unique<NeuralEvaluator>(black->network);
    }
    else {
        evaluators[CL_BLACK] = std::make_unique<QuiesceEvaluator>();
    }

    std::cout << "Game between " << white->id << " and " << black->id << " is STARTING." << std::endl;

    //// Create searchers
    //AlphaBetaSearcher searchers[] = {
    //    AlphaBetaSearcher(std::make_shared<NeuralEvaluator>(white->network)),
    //    AlphaBetaSearcher(std::make_shared<NeuralEvaluator>(black->network)),
    //};

    // Configure time controls
    SearchSettings settings;
    settings.ourTimeControl = tc;
    settings.theirTimeControl = tc;
    settings.maxDepth = maxDepth;

    Color curr = pos.getColorToMove();
    bool flagged = false;
    ChessResult gameRes;

    // Play the game until it ends, either by clock or board
    while ((gameRes = pos.getResult(CL_WHITE, !flagged)) == RES_UNFINISHED) {
        //auto results = searchers[curr].search(pos, settings);

        //TrainingGameMoveData moveData;
        //moveData.move = results.bestMove;
        //moveData.score = results.bestScore;

        auto moveData = agentPickMove(*evaluators[curr], pos);
        pos.makeMove(moveData.move);
        ret->moves.push_back(moveData);
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
    auto& agentsToPlay = gen.agents;

    for (int i = 0; i < agentsToPlay.size() - 1; ++i) {
        for (int j = i; j < agentsToPlay.size(); ++j) {
            Agent* a = agentsToPlay[i];
            Agent* b = agentsToPlay[j];

            for (int k = 0; k < m_Settings.matchesPerPairing; ++k) {
                pairings.emplace_back(a, b);
                pairings.emplace_back(b, a);
            }
        }
    }

    return pairings;
}

constexpr int WIN_FITNESS = 10000;

void GeneticTraining::playGames(const GamePairings& pairings) {

    for (const auto& pair: pairings) {
        Agent* white = pair.first;
        Agent* black = pair.second;
        auto game = playAgentGame(white, black, m_Settings.timeControl, m_Settings.maxDepth);

        // Update fitness scores
        if (isDraw(game->resultForWhite)) {
            // Draws don't affect fitness
            m_Games[game->id] = std::move(game);
            continue;
        }

        // Game ended with decisive result
        Agent* winner; Agent* loser;
        if (isWin(game->resultForWhite)) {
            winner = white;
            loser = black;
        }
        else {
            winner = black;
            loser = white;
        }
        // Do not mistake moves for plies!
        int moveCount = game->moves.size() / 2;

        m_CurrGeneration.agentFitness[loser->id] += moveCount - WIN_FITNESS;
        m_CurrGeneration.agentFitness[winner->id] += WIN_FITNESS - moveCount;

        m_Games[game->id] = std::move(game);
    }
}

void GeneticTraining::cull() {
    // Sort agents from fittest to less fit
    std::sort(m_CurrGeneration.agents.begin(), m_CurrGeneration.agents.end(),
              [this](auto a, auto b) {
        return m_CurrGeneration.agentFitness[a->id] > m_CurrGeneration.agentFitness[b->id];
    });

    // Now, remove all unfit agents
    int targetSize = m_CurrGeneration.agents.size() - m_Settings.cullingRate;
    for (int i = m_CurrGeneration.agents.size() - 1; i >= targetSize; --i) {
        Agent* agent = m_CurrGeneration.agents[i];
        m_CurrGeneration.agentFitness.erase(agent->id);
        m_CurrGeneration.agents.erase(m_CurrGeneration.agents.begin() + i);
        m_Agents.erase(agent->id);
    }

    m_Games.clear();
}

static std::unique_ptr<Agent> crossoverAgents(int genNumber, const Agent& a, const Agent& b) {
    auto ret = std::make_unique<Agent>();

    ret->id = randomId();
    if (a.network == nullptr && b.network != nullptr) {
        ret->network = std::make_shared<NN>(*b.network);
    }
    else if (b.network == nullptr && a.network != nullptr) {
        ret->network = std::make_shared<NN>(*a.network);
    }
    else if (a.network == nullptr) {
        ret->network = std::make_shared<NN>();
        ret->network->randomize();
    }
    else {
        ret->network = std::make_shared<NN>(*a.network, *b.network);
    }
    ret->generationNumber = genNumber;

    return ret;
}

static void mutateAgent(Agent& a, float mutationRatePct) {
    if (a.network == nullptr) {
        return;
    }
    a.network->mutate(mutationRatePct);
}

void GeneticTraining::reproduceAgents() {
    m_CurrGeneration.number++;

    // Perform crossovers
    std::vector<Agent*> parentPool(m_CurrGeneration.agents.begin(), m_CurrGeneration.agents.end());

    while (m_CurrGeneration.agents.size() < m_Settings.agentsPerGeneration) {
        // Get two random parents from the pool
        size_t rnd = utils::random(static_cast<size_t>(0), parentPool.size());
        auto parentA = parentPool[rnd];
        Agent* parentB;

        do {
            rnd = utils::random(static_cast<size_t>(0), parentPool.size());
            parentB = parentPool[rnd];
        } while(parentB == parentA);

        // Crossover the parents, generating a new agent.
        auto agent = crossoverAgents(m_CurrGeneration.number, *parentA, *parentB);
        mutateAgent(*agent, m_CurrMutRate);
        m_CurrGeneration.agents.push_back(agent.get());
        m_CurrGeneration.agentFitness[agent->id] = 0;

        m_Agents[agent->id] = std::move(agent);
    }

    m_CurrMutRate = std::max(m_CurrMutRate - m_Settings.mutationRatePerGen, m_Settings.minMutationRate);
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
    if (a.network != nullptr) {
        j["network"] = *a.network;
    }
    else {
        j["network"] = nullptr;
    }
}

void from_json(const nlohmann::json& j, Agent& a) {
    a.id = j.get<std::string>();
    a.generationNumber = j.get<int>();
    if (!j.at("network").is_null()) {
        a.network = std::make_shared<NN>(j.get<NN>());
    }
    else {
        a.network = nullptr;
    }
}

void to_json(nlohmann::json& j, const Generation& gen) {
    j["number"] = gen.number;

    // Fetch agent ids
    std::unordered_map<std::string, int> agentsAndFitness;
    for (const auto& ag: gen.agents) {
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