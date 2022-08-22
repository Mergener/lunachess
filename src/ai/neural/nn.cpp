#include "nn.h"

#include <algorithm>
#include <cstring>
#include <mmintrin.h>
#include "../../position.h"

namespace lunachess::ai::neural {

float activationFunction(float x) {
    return std::max(0.0f, (x - 0.5f) * 2);
}

template <int N, int INPUT_SIZE>    
class NetworkLayer {
public:

    static float randomFloat() {
        return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

    }

    NetworkLayer() {
        //std::memset(weights, 0, sizeof(weights));
        //std::memset(biases, 0, sizeof(biases));

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
                result.weights[i][j] = (rand() % 2 == 0) ? a.weights[i][j] : b.weights[i][j];
            }
            result.biases[i] = (rand() % 2 == 0) ? a.biases[i] : b.biases[i];
        }
        return result;
    }
    
    void mutate(int mutationRatePct) {
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < INPUT_SIZE; ++j) {
                if (rand() % 100 < mutationRatePct) {
                    m_Weights[i][j] += (rand() % 1000 - 500) / 1000.0f;
                }
            }
            if (rand() % 100 < mutationRatePct) {
                m_Biases[i] += (rand() % 1000 - 500) / 1000.0f;
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

private:
    NetworkLayer<N_HIDDEN_NEURONS, N_INPUTS> m_FirstHidden;
    NetworkLayer<N_HIDDEN_NEURONS, N_HIDDEN_NEURONS> m_HiddenLayers[N_INBETWEEN_LAYERS];
    NetworkLayer<1, N_HIDDEN_NEURONS> m_OutputLayer;
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


};

