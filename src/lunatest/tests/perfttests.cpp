#include "../lunatest.h"

#include <lunachess.h>

#include <vector>
#include <initializer_list>

namespace lunachess::tests {

struct PerftTests : public TestSuite {
    struct PerftTest {
        Position pos;
        std::vector<int> expectedNodes;

        PerftTest(std::string_view fen, const std::initializer_list<int>& expectedNodes)
            : pos(Position::fromFen(fen).value()),
            expectedNodes(expectedNodes) {
        }
    };

    std::vector<PerftTest> tests = {
        PerftTest("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", { 20, 400, 8902, 197281, 4865609, 119060324 }),
        PerftTest("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ", { 48, 2039, 97862, 4085603 }),
        PerftTest("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ", { 14, 191, 2812, 43238, 674624 }),
    };

    void run() override {
        for (const auto& t: tests) {
            for (int i = 0; i < t.expectedNodes.size(); ++i) {
                int expectedNodes = t.expectedNodes[i];
                int depth = i + 1;
                ui64 result = perft(t.pos, depth, false, false, false);
                LUNA_ASSERT(result == expectedNodes,
                            "Expected " << expectedNodes << ", got " << result << " for perft " << depth
                            << " at pos '" << t.pos.toFen());
            }
        }
    }

    PerftTests()
    : TestSuite("perft") {}
};

}