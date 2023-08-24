#include "../lunatest.h"

namespace lunachess::tests {

static constexpr Color CL_ANY = CL_COUNT;

struct EndgameTest {
    std::string fen;
    EndgameData expectedEG;

    EndgameTest(std::string_view fen, EndgameType expectedType, Color expectedLhs)
            : fen(fen) {
        expectedEG.lhs = expectedLhs;
        expectedEG.type = expectedType;
    }

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        EndgameData egData = endgame::identify(pos);

        LUNA_ASSERT(egData.type == expectedEG.type,
                    "Expected endgame type " << i32(expectedEG.type) << ", got " << i32(egData.type));

        if (expectedEG.lhs != CL_ANY) {
            LUNA_ASSERT(egData.lhs == expectedEG.lhs,
                        "Expected endgame lhs color " << getColorName(expectedEG.lhs) << ", got " << getColorName(egData.lhs));
        }
    }
};

std::vector<TestCase> endgameTests = {
    EndgameTest("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", EG_UNKNOWN, CL_ANY),
    EndgameTest("8/8/3k4/8/8/2PK4/8/8 w - - 0 1", EG_KP_K, CL_WHITE),
    EndgameTest("8/8/2pk4/8/8/3K4/8/8 b - - 0 1", EG_KP_K, CL_BLACK),
    EndgameTest("8/8/3k4/6B1/8/3K4/1N6/8 b - - 0 1", EG_KBN_K, CL_WHITE),
    EndgameTest("8/8/1bnk4/8/8/3K4/8/8 b - - 0 1", EG_KBN_K, CL_BLACK),
    EndgameTest("8/8/3k4/8/8/8/3RK3/8 b - - 0 1", EG_KR_K, CL_WHITE),
    EndgameTest("8/8/3k1r2/8/8/8/4K3/8 w - - 0 1", EG_KR_K, CL_BLACK),
    EndgameTest("8/8/3k4/8/7Q/8/4K3/8 w - - 0 1", EG_KQ_K, CL_WHITE),
    EndgameTest("8/8/3k4/8/7q/8/4K3/8 w - - 0 1", EG_KQ_K, CL_BLACK),
    EndgameTest("8/8/3k4/8/6pq/8/4K3/8 w - - 0 1", EG_UNKNOWN, CL_ANY),
};

}