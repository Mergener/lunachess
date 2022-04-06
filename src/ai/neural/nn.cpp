#include "nn.h"

#include <array>
#include <cstring>
#include <immintrin.h>

#include "../../types.h"
#include "../../position.h"

namespace lunachess::ai::neural {

static int reLu(int val) {
    return val > 0 ? val : 0;
}

template <int N, int NINPUTS>
class HiddenLayer {
public:
    std::array<i32, N> feedForward(const std::array<i32, NINPUTS>& inputs) {
        std::array<i32, N> ret;
        for (int i = 0; i < N; ++i) {
            ret[i] = 0;
            for (int j = 0; j < NINPUTS; ++j) {
                ret[i] += activationFunction(m_Weights[i][j] * inputs[j] + m_Biases[i]);
            }
        }

        return ret;
    }

    inline int activationFunction(i32 val) {
        return reLu(val);
    }

    inline HiddenLayer() {
        std::memset(m_Weights, 0, sizeof(m_Weights));
        std::memset(m_Biases, 0, sizeof(m_Biases));
    }

private:
    i32 m_Weights[N][NINPUTS];
    i32 m_Biases[N];
};

class EvaluationNetwork {
    static constexpr int N_INPUTS = 68;

public:
    i32 evaluate(const Position& pos) {

    }

private:
    std::array<i32, N_INPUTS> asd;
};

}