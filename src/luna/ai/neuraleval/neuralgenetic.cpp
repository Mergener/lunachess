#include "neuralgenetic.h"

#include <cmath>
#include <future>
#include <thread>
#include <fstream>
#include <istream>
#include <iostream>

#include "../quiescevaluator.h"

#include "../../strutils.h"

namespace lunachess::ai::neural {

namespace fs = std::filesystem;

static std::ostream& operator<<(std::ostream& stream, const Agent& a) {
    stream << "(id: " << a.id << ", loss: " << a.avgLoss << ")";
    return stream;
}

static std::ostream& operator<<(std::ostream& stream, const Agent* a) {
    stream << *a;
    return stream;
}

static int cpToQ(int evalCp) {
    double evalCpF = evalCp;
    double retF = 0.64017665102 * std::atan(0.0089513781885 * evalCpF);
    return static_cast<int>(retF * 1024);
}

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

static int clamp(int val, int min, int max) {
    if (val > max) {
        return max;
    }
    if (val < min) {
        return min;
    }
    return val;
}

[[noreturn]] void GeneticTraining::run() {
    std::cout << "Initializing genetic training." << std::endl;
    std::cout << "The output path is '" << m_Settings.savePath << "'" << std::endl;
    std::cout << "The input path is '" << m_Settings.datasetPath << "'" << std::endl;

    install();
    loadDataset();

    m_CurrMutRate = m_Settings.initialMutationRate;

    // Create missing agents
    std::cout << "Generating initial agent pool..." << std::endl;
    int nMissingAgts = m_Settings.agentsPerGen - m_Agents.size();
    createRandomAgents(nMissingAgts);
    std::cout << nMissingAgts << " random agents generated." << std::endl;

    std::cout << "Genetic training is running." << std::endl;
    while (true) {
        std::cout << "Starting generation " << m_CurrGen.number << std::endl;
        simulate();
        selectSome();
        reproduce();
        save();

        m_CurrGen.number++;
        m_CurrMutRate = clamp(m_CurrMutRate + m_Settings.mutationRatePerGen,
                              m_Settings.minMutationRate, m_Settings.maxMutationRate);
    }
}

void GeneticTraining::install() {
    fs::create_directories(m_Settings.savePath);
}

void GeneticTraining::loadDataset() {
    std::cout << "Loading dataset..." << std::endl;
    for (int i = 0; i < m_Settings.nDataFiles; ++i) {
        std::ifstream ifstream(m_Settings.datasetPath / fs::path(strutils::toString(i + 1) + std::string(".json")));

        std::stringstream buffer;
        buffer << ifstream.rdbuf();
        ifstream.close();
        std::string s = buffer.str();

        std::vector<ReferenceData> fileData = nlohmann::json::parse(s);

        for (const auto& ref: fileData) {
            m_Dataset.push_back(ref);
        }
    }
    std::cout << "Finished loading dataset." << std::endl;
}

template <int N, int N_INPUTS, ActivationFunctionType FN>
static void randomizeNNLayer(NNLayer<N, N_INPUTS, FN>& layer) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N_INPUTS; ++j) {
            auto& w = layer.weights[i][j];
            w = utils::random(-16, 16);
        }
    }
    for (int i = 0; i < N; ++i) {
        auto& b = layer.biases[i];
        b = utils::random(-24, 24);
    }
}

static void randomizeNN(EvalNN& nn) {
    randomizeNNLayer(nn.w1);
    randomizeNNLayer(nn.w2);
    randomizeNNLayer(nn.w3);
    randomizeNNLayer(nn.w4);
}

void GeneticTraining::createRandomAgents(int n) {
    for (int i = 0; i < n; ++i) {
        auto agent = std::make_unique<Agent>();

        agent->id = randomId();
        agent->nn = std::make_shared<EvalNN>();

        randomizeNN(*agent->nn);

        m_Agents[agent->id] = std::move(agent);
    }
}

void GeneticTraining::simulate() {
    std::vector<std::thread> jobs;

    std::cout << "Running simulations..." << std::endl;
    for (const auto& pair: m_Agents) {
        if (pair.second->simulated) {
            continue;
        }
        pair.second->avgLoss = 0;
        pair.second->simulated = true;
        jobs.emplace_back([&]() {
            simulateAgent(*pair.second);
        });
    }

    for (auto& j: jobs) {
        j.join();
    }
    std::cout << "Simulations finished." << std::endl;
}

void GeneticTraining::simulateAgent(Agent& a) {
    QuiesceEvaluator eval(std::make_shared<NeuralEvaluator>(a.nn));
    ui64 totalLoss = 0;

    for (const auto& data: m_Dataset) {
        Position pos = Position::fromFen(data.fen).value();

        int refEval = cpToQ(data.eval);
        int agentEval = cpToQ(eval.evaluate(pos));

        int delta = refEval - agentEval;
        int absDelta = std::abs(delta);

        totalLoss += absDelta;
    }

    a.avgLoss = totalLoss / m_Dataset.size();
    std::cout << "Agent '" << a.id << "' avg loss: " << a.avgLoss << std::endl;
}

void GeneticTraining::selectSome() {
    std::vector<Agent*> agents;

    std::sort(agents.begin(), agents.end(), [](auto a, auto b) {
       return a->avgLoss < b->avgLoss;
    });

    std::cout << "Best so far: " << agents[0]->avgLoss << std::endl;

    for (int i = m_Settings.cutPoint; i < agents.size(); ++i) {
        Agent* a = agents[i];
        m_Agents.erase(a->id);
    }
}

void GeneticTraining::reproduce() {
    std::vector<Agent*> agents;

    for (auto& pair: m_Agents) {
        agents.push_back(pair.second.get());
    }

    std::sort(agents.begin(), agents.end(), [](auto a, auto b) {
        return a->avgLoss < b->avgLoss;
    });

    for (int i = 0; i < agents.size() - 1; ++i) {
        for (int j = i + 1; j < agents.size(); ++j) {
            if (m_Agents.size() >= m_Settings.agentsPerGen) {
                break;
            }

            crossoverAgents(*agents[i], *agents[j]);
        }
    }
}

template <int N, int N_INPUTS, ActivationFunctionType FN>
static void crossoverNNLayer(NNLayer<N, N_INPUTS, FN>& target,
                             const NNLayer<N, N_INPUTS, FN>& a,
                             const NNLayer<N, N_INPUTS, FN>& b) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N_INPUTS; ++j) {
            target.weights[i][j] = utils::randomBool() ? a.weights[i][j] : b.weights[i][j];
        }
    }
    for (int i = 0; i < N; ++i) {
        target.biases[i] = utils::randomBool() ? a.biases[i] : b.biases[i];
    }
}

template <int N, int N_INPUTS, ActivationFunctionType FN>
static void mutateNNLayer(NNLayer<N, N_INPUTS, FN>& layer,
                          float mutRate, i32 deltaWeight,
                          i32 deltaBias, i32 weightPctChange,
                          i32 biasPctChange) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N_INPUTS; ++j) {
            auto& w = layer.weights[i][j];

            if (utils::randomChance(mutRate)) {
                if (utils::randomBool()) {
                    w = w * (100 + (utils::randomBool() ? weightPctChange : -weightPctChange)) / 100;
                }
                else {
                    w = w + (utils::randomBool() ? deltaWeight : -deltaWeight);
                }
            }
        }
    }
    for (int i = 0; i < N; ++i) {
        auto& b = layer.biases[i];
        if (utils::randomChance(mutRate)) {
            if (utils::randomBool()) {
                b = b * (100 + (utils::randomBool() ? biasPctChange : -biasPctChange)) / 100;
            }
            else {
                b = b + (utils::randomBool() ? deltaBias : -deltaBias);
            }
        }
    }
}

void GeneticTraining::crossoverAgents(Agent& a, Agent& b) {
    auto agent = std::make_unique<Agent>();

    agent->id = randomId();
    agent->nn = std::make_shared<EvalNN>();

    crossoverNNLayer(agent->nn->w1, a.nn->w1, b.nn->w1);
    crossoverNNLayer(agent->nn->w2, a.nn->w2, b.nn->w2);
    crossoverNNLayer(agent->nn->w3, a.nn->w3, b.nn->w3);
    crossoverNNLayer(agent->nn->w4, a.nn->w4, b.nn->w4);

    mutateNNLayer(agent->nn->w1, m_CurrMutRate, m_Settings.mutDeltaWeight,
                  m_Settings.mutDeltaBias, m_Settings.mutWeightPctChange,
                  m_Settings.mutBiasPctChange);

    mutateNNLayer(agent->nn->w2, m_CurrMutRate, m_Settings.mutDeltaWeight,
                  m_Settings.mutDeltaBias, m_Settings.mutWeightPctChange,
                  m_Settings.mutBiasPctChange);

    mutateNNLayer(agent->nn->w3, m_CurrMutRate, m_Settings.mutDeltaWeight,
                  m_Settings.mutDeltaBias, m_Settings.mutWeightPctChange,
                  m_Settings.mutBiasPctChange);

    mutateNNLayer(agent->nn->w4, m_CurrMutRate, m_Settings.mutDeltaWeight,
                  m_Settings.mutDeltaBias, m_Settings.mutWeightPctChange,
                  m_Settings.mutBiasPctChange);

    m_Agents[agent->id] = std::move(agent);
}

void GeneticTraining::save() {
    std::string genName = strutils::toString(m_CurrGen.number);
    fs::create_directories(m_Settings.savePath / "agents" / genName);
    std::cout << "Saving agents..." << std::endl;
    for (const auto& pair: m_Agents) {
        const auto& a = pair.second;

        std::ofstream stream(m_Settings.savePath / "agents" / genName / (a->id + "_avgloss_" + strutils::toString(a->avgLoss) + ".json"));
        stream << nlohmann::json(*a->nn) << std::endl;
        stream.close();
    }
    std::cout << std::endl;
}


}