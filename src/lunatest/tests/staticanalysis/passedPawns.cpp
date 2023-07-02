#include "staticanalysis.h"

namespace lunachess::tests {

struct PassedPawnsTest {
    std::string fen;
    Color color;
    Bitboard expectedBB;

    PassedPawnsTest(std::string fen, Color color, Bitboard expectedBB)
        : fen(fen), color(color), expectedBB(expectedBB) {}

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        Bitboard result = staticanalysis::getPassedPawns(pos, color);

        LUNA_ASSERT(result == expectedBB,
                    "Expected:\n" << expectedBB << "\n\nGot:\n" << result);
    }
};


std::vector<TestCase> passedPawnsTests = {
    PassedPawnsTest("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1", CL_WHITE, 0),
    PassedPawnsTest("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1", CL_BLACK, 0),
    PassedPawnsTest("8/8/5k2/6p1/8/2PK4/8/8 w - - 0 1", CL_WHITE, 0x40000),
    PassedPawnsTest("8/8/5k2/6p1/8/2PK4/8/8 w - - 0 1", CL_BLACK, 0x4000000000),
    PassedPawnsTest("8/8/1p3k2/6p1/8/2PK4/8/8 w - - 0 1", CL_WHITE, 0),
    PassedPawnsTest("8/8/1p3k2/6p1/8/2PK4/8/8 w - - 0 1", CL_BLACK, 0x4000000000),
    PassedPawnsTest("rnbqkbnr/pp1ppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_WHITE, 0),
    PassedPawnsTest("rnbqkbnr/2pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_WHITE, 0x100),
    PassedPawnsTest("rnbqkbnr/pppppppp/8/8/8/8/2PPPPPP/RNBQKBNR w KQkq - 0 1", CL_BLACK, 0x1000000000000),
    PassedPawnsTest("rnbqkbnr/pppppppp/8/8/8/8/PPPPPP2/RNBQKBNR w KQkq - 0 1", CL_BLACK, 0x80000000000000),
};

}