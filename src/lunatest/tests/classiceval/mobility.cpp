#include "../../lunatest.h"

namespace lunachess::tests {

static constexpr int KNIGHT_MOB = 0x1;
static constexpr int BISHOP_MOB = 0x10;
static constexpr int ROOK_MOB   = 0x100;
static constexpr int QUEEN_MOB  = 0x1000;

static ai::ClassicEvaluator& getMobilityEvaluator() {
    static ai::ClassicEvaluator* eval = nullptr;
    if (eval == nullptr) {
        ai::ScoreTable scores;
        scores.mobilityScores[PT_KNIGHT] = KNIGHT_MOB;
        scores.mobilityScores[PT_BISHOP] = BISHOP_MOB;
        scores.mobilityScores[PT_ROOK] = ROOK_MOB;
        scores.mobilityScores[PT_QUEEN] = QUEEN_MOB;
        eval = new ai::ClassicEvaluator(scores, scores);
    }
    return *eval;
}

struct MobilityTest {
    std::string fen;
    Color color;
    int expectedEval;

    MobilityTest(std::string_view fen, Color color, int expectedEval)
            : fen(fen),
              color(color),
              expectedEval(expectedEval) {}

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        int result = getMobilityEvaluator().evaluateMobility(pos, color, 100);

        LUNA_ASSERT(result == expectedEval,
                    std::hex << "Expected mobility eval at position " << fen << " to be " << expectedEval << " for color " <<
                                                                   getColorName(color) << ", got " << result << ".");
    }
};

std::vector<TestCase> mobilityTests = {
    MobilityTest("8/3KQ3/8/1P2p3/6n1/3b4/4k3/8 w - - 0 1", CL_WHITE, 16 * QUEEN_MOB),
    MobilityTest("8/3KQ3/8/1P2p3/6n1/3b4/4k3/8 w - - 0 1", CL_BLACK, 6 * KNIGHT_MOB + 9 * BISHOP_MOB),
};

} // lunachess::tests