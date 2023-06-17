#include "staticanalysis.h"

namespace lunachess::tests {

struct BackwardPawnTest {
    std::string fen;
    Color color;
    Bitboard expectedBB;

    BackwardPawnTest(std::string fen, Color color, Bitboard expectedBB)
        : fen(fen), color(color), expectedBB(expectedBB) {}

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        Bitboard result = staticanalysis::getBackwardPawns(pos, color);

        LUNA_ASSERT(result == expectedBB,
                    "Expected:\n" << expectedBB << "\n\nGot:\n" << result);
    }
};


std::vector<TestCase> backwardPawnsTests = {
    BackwardPawnTest("8/8/3p4/3P4/2P3k1/8/5K2/8 w - - 0 1", CL_WHITE, 0x4000000),
    BackwardPawnTest("8/8/3p4/2PP4/6k1/8/5K2/8 w - - 0 1", CL_WHITE, 0),
    BackwardPawnTest("8/8/2Pp4/3P4/6k1/8/5K2/8 w - - 0 1", CL_WHITE, 0),
    BackwardPawnTest("8/8/3p4/2p5/2P3k1/8/5K2/8 w - - 0 1", CL_WHITE, 0),
};

}