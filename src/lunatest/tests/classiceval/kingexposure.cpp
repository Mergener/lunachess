#include "../../lunatest.h"

namespace lunachess::tests {

static ai::HandCraftedEvaluator<>& getKingExposureEvaluator() {
    static ai::HandCraftedEvaluator<>* eval = nullptr;
    if (eval == nullptr) {
        ai::ScoreTable scores;
        scores.kingExposureScores[PT_BISHOP] = 1;
        scores.kingExposureScores[PT_ROOK] = 10;
        scores.kingExposureScores[PT_QUEEN] = 100;
        eval = new ai::HandCraftedEvaluator<>(scores, scores);
    }
    return *eval;
}

struct KingExposureTest {
    std::string fen;
    Color color;
    int expectedEval;

    KingExposureTest(std::string_view fen, Color color, int expectedEval)
            : fen(fen),
              color(color),
              expectedEval(expectedEval) {}

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        auto& evaluator = getKingExposureEvaluator();
        evaluator.setPosition(pos);
        int result = evaluator.evaluate();

        std::cout << getPieceTypeName(pos.getPieceAt(SQ_C2).getType()) << std::endl;

        LUNA_ASSERT(result == expectedEval,
                    "Expected king exposure eval at position " << fen << " to be " << expectedEval << " for color " <<
                                                                   getColorName(color) << ", got " << result << ".");
    }
};

std::vector<TestCase> kingExposureTests = {
    KingExposureTest("6k1/5pp1/7p/8/8/8/4Q3/3K4 w - - 0 1", CL_WHITE, 0),
    KingExposureTest("6k1/6p1/5p1p/8/8/8/4Q3/3K4 w - - 0 1", CL_WHITE, 100),
    KingExposureTest("6k1/6p1/5p1p/8/8/8/2B1Q3/3K4 w - - 0 1", CL_WHITE, 101),
    KingExposureTest("6k1/6p1/5p1p/8/8/8/3BQ3/3K4 w - - 0 1", CL_WHITE, 100),
};


}