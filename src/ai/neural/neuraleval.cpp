#include "nn.h"

#include <algorithm>
#include <cstring>
#include <mmintrin.h>
#include <random>
#include <vector>
#include "../../position.h"

namespace lunachess::ai::neural {

static std::mt19937_64 s_Random (std::random_device{}());

static float activationFunction(float x) {
    return std::max(0.0f, x);
}

/**
 * Returns a random float between 0 and 1.
 */
static float randomFloat() {
    return static_cast<float>(s_Random()) / static_cast<float>(s_Random.max());
}

template <int N, int INPUT_SIZE>    
class NetworkLayer {
public:

    NetworkLayer(bool randomize = true) {
        if (!randomize) {
            return;
        }

        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < INPUT_SIZE; ++j) {
                m_Weights[i][j] = randomFloat();
            }
            m_Biases[i] = randomFloat();
        }
    }

    static NetworkLayer crossover(const NetworkLayer& a, const NetworkLayer& b) {
        NetworkLayer result;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < INPUT_SIZE; ++j) {
                result.m_Weights[i][j] = (s_Random() % 2 == 0) ? a.weights[i][j] : b.weights[i][j];
            }
            result.m_Biases[i] = (s_Random() % 2 == 0) ? a.biases[i] : b.biases[i];
        }
        return result;
    }
    
    void mutate(int mutationRatePct) {
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < INPUT_SIZE; ++j) {
                if (s_Random() % 100 < mutationRatePct) {
                    m_Weights[i][j] += (s_Random() % 1000 - 500) / 1000.0f;
                }
            }
            if (s_Random() % 100 < mutationRatePct) {
                m_Biases[i] += (s_Random() % 1000 - 500) / 1000.0f;
            }
        }
    }

    void feedForward(const float* source, float* dest) const {
        for (int i = 0; i < N; ++i) {
            float neuronSum = 0;
            for (int j = 0; j < INPUT_SIZE; ++j) {
                neuronSum += source[j] * m_Weights[i][j];
            }
            dest[i] = activationFunction(neuronSum + m_Biases[i]);
        }
    }


private:
    float m_Weights[N][INPUT_SIZE];
    float m_Biases[N];
};

struct Inputs {
    float pieces[64]; // 0 for no pieces at square, >0 for our pieces, <0 for opponent pieces

    float castleRightsUs; // 0 for no castling rights, 1 for O-O, 2 for O-O-O, 3 for O-O and O-O-O
    float castleRightsThem; // same as above

    float epSquare;

    operator float*() {
        return std::launder(reinterpret_cast<float*>(this));
    }

    operator const float*() const {
        return std::launder(reinterpret_cast<const float*>(this));
    }

    Inputs() = default;
    ~Inputs() = default;

    Inputs(const Position& pos)
        : Inputs(pos, pos.getColorToMove()) {
    }

    Inputs(const Position& pos, Color us) {
        std::memset(pieces, 0, sizeof(pieces));
        
        // Pieces
        for (Square s = 0; s < 64; ++s) {
            Piece p = pos.getPieceAt(s);
            pieces[s] = p.getType();
            if (p.getColor() != us) {
                pieces[s] *= -1;
            }
        }

        // Castling rights
        castleRightsUs = 0;
        if (pos.getCastleRights(us, SIDE_KING)) {
            castleRightsUs += 1;
        }
        if (pos.getCastleRights(us, SIDE_QUEEN)) {
            castleRightsUs += 2;
        }

        castleRightsThem = 0;
        if (pos.getCastleRights(getOppositeColor(us), SIDE_KING)) {
            castleRightsThem += 1;
        }
        if (pos.getCastleRights(getOppositeColor(us), SIDE_QUEEN)) {
            castleRightsThem += 2;
        }

        // En passant square
        epSquare = pos.getEnPassantSquare();
    }    
};

static constexpr int N_INPUTS = sizeof(Inputs)/sizeof(float);

class NeuralNetwork {
    static constexpr int N_HIDDEN_NEURONS = 64;
    static constexpr int N_HIDDEN_LAYERS = 2;

    static constexpr int N_INBETWEEN_LAYERS = N_HIDDEN_LAYERS - 1;

    using TFirstHidden = NetworkLayer<N_HIDDEN_NEURONS, N_INPUTS>;
    using THidden      = NetworkLayer<N_HIDDEN_NEURONS, N_HIDDEN_NEURONS>;
    using TOut         = NetworkLayer<1, N_HIDDEN_NEURONS>;
public:

    int evaluate(const Position& pos) const {
        Color us = pos.getColorToMove();
        Inputs inputs(pos, us);

        float a[N_HIDDEN_NEURONS], b[N_HIDDEN_NEURONS];
        float* src = a;
        float* dst = b;

        // Feed to first hidden layers
        m_FirstHidden.feedForward(inputs, a);

        // Subsequently feed to next layers
        for (int i = 0; i < N_INBETWEEN_LAYERS; ++i) {
            m_HiddenLayers[i].feedForward(src, dst);
            std::swap(src, dst);
        }

        // Feed to the output layer
        m_OutputLayer.feedForward(src, dst);

        // Result is dst[0], multiply by 100 to get value at centipawns.
        return dst[0];
    }

    void mutate(int mutationRatePct) {
        m_FirstHidden.mutate(mutationRatePct);
        for (int i = 0; i < N_INBETWEEN_LAYERS; ++i) {
            m_HiddenLayers[i].mutate(mutationRatePct);
        }
        m_OutputLayer.mutate(mutationRatePct);
    }

    static NeuralNetwork crossover(const NeuralNetwork& a, const NeuralNetwork& b) {
        NeuralNetwork ret;

        ret.m_FirstHidden = TFirstHidden::crossover(a.m_FirstHidden, b.m_FirstHidden);
        for (int i = 0; i < N_INBETWEEN_LAYERS; ++i) {
            ret.m_HiddenLayers[i] = THidden::crossover(a.m_HiddenLayers[i], b.m_HiddenLayers[i]);
        }
        ret.m_OutputLayer = TOut::crossover(a.m_OutputLayer, b.m_OutputLayer);

        return ret;
    }

private:
    TFirstHidden m_FirstHidden;
    THidden      m_HiddenLayers[N_INBETWEEN_LAYERS];
    TOut         m_OutputLayer;
};

int NeuralEvaluator::evaluate(const Position& pos) const {
    return m_Network->evaluate(pos);
}

NeuralEvaluator::NeuralEvaluator() {
    m_Network = new NeuralNetwork();
}

NeuralEvaluator::~NeuralEvaluator() {
    delete m_Network;
}

class Agent {
public:

    inline void addScore() { m_Score++; }
    inline void reduceScore() { m_Score--; }

    inline const NeuralEvaluator& getEvaluator() const { return m_Eval; }

private:
    NeuralEvaluator m_Eval;
    int m_Score = 0;
};

struct Generation {
    int generationNumber;
    std::vector<Agent> agents;  

    Generation(int nAgents) {
        agents.resize(nAgents);
    }
};

struct GeneticTrainingContext {
    Generation currentGeneration;

    GeneticTrainingContext(const GeneticTrainingSettings& settings)
        : currentGeneration(settings.agentsPerGeneration) {
    }
};

void performGeneticTraining(const GeneticTrainingSettings& settings) {
    GeneticTrainingContext ctx(settings);
    
    std::cout << "Generated agents: " << std::endl;

    for (const auto& ag: ctx.currentGeneration.agents) {
        std::cout << std::endl;
    }
}

};

