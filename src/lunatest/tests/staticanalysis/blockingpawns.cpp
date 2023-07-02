#include "staticanalysis.h"

namespace lunachess::tests {

struct BlockingPawnsTest {
    std::string fen;
    Color color;
    Bitboard expectedBB;

    BlockingPawnsTest(std::string fen, Color color, Bitboard expectedBB)
        : fen(fen), color(color), expectedBB(expectedBB) {}

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        Bitboard result = staticanalysis::getBlockingPawns(pos, color);

        LUNA_ASSERT(result == expectedBB,
                    "Expected:\n" << expectedBB << "\n\nGot:\n" << result);
    }
};


std::vector<TestCase> blockingPawnsTests = {
    BlockingPawnsTest("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_WHITE, 0),
    BlockingPawnsTest("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_BLACK, 0),
    BlockingPawnsTest("rnbqkbnr/pppppppp/8/8/8/4P3/PPP1PPPP/RNBQKBNR w KQkq - 0 1", CL_WHITE, 0x100000),
    BlockingPawnsTest("rnbqkbnr/pppppppp/8/8/8/4P3/PPP1PPPP/RNBQKBNR w KQkq - 0 1", CL_BLACK, 0),
    BlockingPawnsTest("rnbqkbnr/ppp2ppp/4p3/4p3/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_WHITE, 0),
    BlockingPawnsTest("rnbqkbnr/ppp2ppp/4p3/4p3/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_BLACK, 0x1000000000),
    BlockingPawnsTest("rnbqkbnr/pppp1p1p/5p2/1P3p1P/8/1N3P1P/1PP1PP1P/R1BQKBNR w KQkq - 0 1", CL_WHITE, 0x8200a00000),
};

}