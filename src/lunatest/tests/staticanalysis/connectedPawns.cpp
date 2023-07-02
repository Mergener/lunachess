#include "staticanalysis.h"

namespace lunachess::tests {

struct ConnectedPawnsTest {
    std::string fen;
    Color color;
    Bitboard expectedBB;

    ConnectedPawnsTest(std::string fen, Color color, Bitboard expectedBB)
        : fen(fen), color(color), expectedBB(expectedBB) {}

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        Bitboard result = staticanalysis::getConnectedPawns(pos, color);

        LUNA_ASSERT(result == expectedBB,
                    "Expected:\n" << expectedBB << "\n\nGot:\n" << result);
    }
};


std::vector<TestCase> connectedPawnsTests = {
    ConnectedPawnsTest("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1", CL_WHITE, 0xff00),
    ConnectedPawnsTest("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1", CL_BLACK, 0xff000000000000),
    ConnectedPawnsTest("4k3/pppppppp/8/8/6P1/1P5P/P1P2P2/4K3 w - - 0 1", CL_WHITE, 0x40822500),
    ConnectedPawnsTest("rnbqkbnr/pp1p1ppp/8/8/8/8/PP1PP1PP/RNBQKBNR w KQkq - 0 2", CL_BLACK, 0xe3000000000000),
    ConnectedPawnsTest("8/3kp2p/7P/8/8/5K2/6P1/8 w - - 0 1", CL_WHITE, 0x800000004000),
};

}