#include "../../lunatest.h"

namespace lunachess::tests {

static constexpr int NEAR_PAWN_ATK = 1;
static constexpr int NEAR_KNIGHT_ATK = 5;
static constexpr int NEAR_BISHOP_ATK = 80;
static constexpr int NEAR_ROOK_ATK = 1200;
static constexpr int NEAR_QUEEN_ATK = 60000;
static constexpr int NEAR_KING_ATK = 4000000;

static ai::ClassicEvaluator& getNearKingAttacksEvaluator() {
    static ai::ClassicEvaluator* eval = nullptr;
    if (eval == nullptr) {
        ai::ScoreTable scores;
        scores.nearKingAttacksScore[PT_PAWN] = NEAR_PAWN_ATK;
        scores.nearKingAttacksScore[PT_KNIGHT] = NEAR_KNIGHT_ATK;
        scores.nearKingAttacksScore[PT_BISHOP] = NEAR_BISHOP_ATK;
        scores.nearKingAttacksScore[PT_ROOK] = NEAR_ROOK_ATK;
        scores.nearKingAttacksScore[PT_QUEEN] = NEAR_QUEEN_ATK;
        scores.nearKingAttacksScore[PT_KING] = NEAR_KING_ATK;
        eval = new ai::ClassicEvaluator(scores, scores);
    }
    return *eval;
}

struct NearKingAttacksTest {
    std::string fen;
    Color color;
    int expectedEval;

    NearKingAttacksTest(std::string_view fen, Color color, int expectedEval)
            : fen(fen),
              color(color),
              expectedEval(expectedEval) {}

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        int result = getNearKingAttacksEvaluator().evaluateNearKingAttacks(pos, color, 100);

        LUNA_ASSERT(result == expectedEval,
                    "Expected near king attacks eval at position " << fen << " to be " << expectedEval << " for color " <<
                                                                   getColorName(color) << ", got " << result << ".");
    }
};

std::vector<TestCase> nearKingAttacksTests = {
        NearKingAttacksTest("8/5k2/8/8/8/8/5K2/8 w - - 0 1", CL_WHITE, 0),
        NearKingAttacksTest("8/5k2/8/8/8/8/5K2/8 w - - 0 1", CL_BLACK, 0),
        NearKingAttacksTest("8/5k2/8/8/3B4/8/5K2/8 w - - 0 1", CL_WHITE, 0),
        NearKingAttacksTest("8/5k2/8/8/3B4/8/5K2/8 w - - 0 1", CL_BLACK, 4 * NEAR_BISHOP_ATK),
        NearKingAttacksTest("8/5k2/5p2/8/3B4/8/5K2/8 w - - 0 1", CL_WHITE, 0),
        NearKingAttacksTest("8/5k2/5p2/8/3B4/8/5K2/8 w - - 0 1", CL_BLACK, 2 * NEAR_BISHOP_ATK),
};

} // lunachess::tests