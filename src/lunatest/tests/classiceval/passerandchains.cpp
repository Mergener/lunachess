#include "../../lunatest.h"

namespace lunachess::tests {

struct PasserAndChainTest {
    std::string fen;
    Color color;
    Bitboard expectedPassers;
    Bitboard expectedChainPawns;

    PasserAndChainTest(std::string_view fen, Color color, Bitboard expectedPassers, Bitboard expectedChainPawns)
            : fen(fen),
              color(color),
              expectedPassers(expectedPassers),
              expectedChainPawns(expectedChainPawns) {}

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        Bitboard passers = ai::ClassicEvaluator::getPassedPawns(pos, color);
        Bitboard chainPawns = ai::ClassicEvaluator::getChainPawns(pos, color);

        LUNA_ASSERT(passers == expectedPassers,
                    "At position '" << fen << "', for color " << getColorName(color) <<
                    ", expected passer bitboard 0x" << std::hex << ui64(expectedPassers) << ", got 0x" << ui64(passers));

        LUNA_ASSERT(chainPawns == expectedChainPawns,
                    "At position '" << fen << "', for color " << getColorName(color) <<
                    ", expected pawn chain bitboard 0x" << std::hex << ui64(expectedChainPawns) << ", got 0x" << ui64(chainPawns));
    }
};

std::vector<TestCase> passerAndChainTests = {
    PasserAndChainTest("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_WHITE, 0x0, 0xff00),
    PasserAndChainTest("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", CL_BLACK, 0x0, 0xff000000000000),
    PasserAndChainTest("8/pp2k2p/8/8/8/PP2K3/8/8 w - - 0 1", CL_WHITE, 0x0, 0x30000),
    PasserAndChainTest("8/pp2k2p/8/8/8/PP2K3/8/8 w - - 0 1", CL_BLACK, 0x80000000000000, 0x3000000000000),
    PasserAndChainTest("8/4k3/8/1P2p1Pp/1p2P3/4K3/8/8 w - - 0 1", CL_WHITE, 0x4200000000, 0x0),
    PasserAndChainTest("8/4k3/8/1P2p1Pp/1p2P3/4K3/8/8 w - - 0 1", CL_BLACK, 0x8002000000, 0x0),
    PasserAndChainTest("8/3p4/8/8/3PP3/4p3/1K1k4/8 w - - 0 1", CL_WHITE, 0x0, 0x18000000),
    PasserAndChainTest("8/3p4/8/8/3PP3/4p3/1K1k4/8 w - - 0 1", CL_BLACK, 0x100000, 0x8000000100000),
};


}