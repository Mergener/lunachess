#include "../../lunatest.h"

namespace lunachess::tests {

static ai::ClassicEvaluator& getColorComplexEvaluator() {
    static ai::ClassicEvaluator* eval = nullptr;
    if (eval == nullptr) {
        ai::ScoreTable scores;
        scores.goodComplexScore = 1;
        eval = new ai::ClassicEvaluator(scores, scores);
    }
    return *eval;
}

struct ColorComplexTest {
    std::string fen;
    Color color;
    int expectedEval;

    ColorComplexTest(std::string_view fen, Color color, int expectedEval)
            : fen(fen),
              color(color),
              expectedEval(expectedEval) {}

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        int result = getColorComplexEvaluator().evaluatePawnComplex(pos, color, 100);

        LUNA_ASSERT(result == expectedEval,
                    std::hex << "Expected color complex eval at position " << fen << " to be " << expectedEval << " for color " <<
                                                                   getColorName(color) << ", got " << result << ".");
    }
};

std::vector<TestCase> colorComplexTests = {
    ColorComplexTest("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_WHITE, 0),
    ColorComplexTest("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_BLACK, 0),
    ColorComplexTest("rn1qkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_WHITE, 0),
    ColorComplexTest("rn1qkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_BLACK, 4),
    ColorComplexTest("8/3bb3/4pk2/8/8/2PPP3/3KB3/8 w - - 0 1", CL_WHITE, 2),
    ColorComplexTest("8/3bb3/4pk2/8/8/2PPP3/3KB3/8 w - - 0 1", CL_BLACK, 0),
};

} // lunachess::tests