#include "../../lunatest.h"

#include <lunachess.h>

#include <vector>

namespace lunachess::tests {

static void testPseudoLegal(Position& pos, int depth) {
    if (depth <= 0) {
        return;
    }

    MoveList moves;
    movegen::generate(pos, moves);
    for (Move move: moves) {
        LUNA_ASSERT(pos.isMovePseudoLegal(move),
                    "Expected move " << move << " to be pseudo legal in position " << pos.toFen());

        pos.makeMove(move);
        // Check for legality since we only define behaviors under the boundaries of legal
        // positions. Illegal positions are out of our scope, so we don't care if they produce
        // undefined behavior.
        if (pos.legal()) {
            testPseudoLegal(pos, depth - 1);
        }
        pos.undoMove();
    }
}

struct PseudoLegalityTest {
    std::string fen;
    int depth;

    PseudoLegalityTest(std::string_view fen, int depth)
            : fen(fen),
              depth(depth) {
    }

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        testPseudoLegal(pos, depth);
    }
};

#define DEPTH 4

std::vector<TestCase> pseudoLegalityTests = {
    PseudoLegalityTest("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", DEPTH),
    PseudoLegalityTest("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", DEPTH),
    PseudoLegalityTest("4k3/8/8/8/8/8/8/4K2R w K - 0 1", DEPTH),
    PseudoLegalityTest("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", DEPTH),
    PseudoLegalityTest("4k2r/8/8/8/8/8/8/4K3 w k - 0 1", DEPTH),
    PseudoLegalityTest("r3k3/8/8/8/8/8/8/4K3 w q - 0 1", DEPTH),
    PseudoLegalityTest("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", DEPTH),
    PseudoLegalityTest("r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", DEPTH),
    PseudoLegalityTest("8/8/8/8/8/8/6k1/4K2R w K - 0 1", DEPTH),
    PseudoLegalityTest("8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", DEPTH),
    PseudoLegalityTest("4k2r/6K1/8/8/8/8/8/8 w k - 0 1", DEPTH),
    PseudoLegalityTest("r3k3/1K6/8/8/8/8/8/8 w q - 0 1", DEPTH),
    PseudoLegalityTest("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", DEPTH),
    PseudoLegalityTest("r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", DEPTH),
    PseudoLegalityTest("r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", DEPTH),
    PseudoLegalityTest("r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", DEPTH),
    PseudoLegalityTest("1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", DEPTH),
    PseudoLegalityTest("2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", DEPTH),
    PseudoLegalityTest("r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", DEPTH),
    PseudoLegalityTest("4k3/8/8/8/8/8/8/4K2R b K - 0 1", DEPTH),
    PseudoLegalityTest("4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", DEPTH),
    PseudoLegalityTest("4k2r/8/8/8/8/8/8/4K3 b k - 0 1", DEPTH),
    PseudoLegalityTest("r3k3/8/8/8/8/8/8/4K3 b q - 0 1", DEPTH),
    PseudoLegalityTest("4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", DEPTH),
    PseudoLegalityTest("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", DEPTH),
    PseudoLegalityTest("8/8/8/8/8/8/6k1/4K2R b K - 0 1", DEPTH),
    PseudoLegalityTest("8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", DEPTH),
    PseudoLegalityTest("4k2r/6K1/8/8/8/8/8/8 b k - 0 1", DEPTH),
    PseudoLegalityTest("r3k3/1K6/8/8/8/8/8/8 b q - 0 1", DEPTH),
    PseudoLegalityTest("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", DEPTH),
    PseudoLegalityTest("r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", DEPTH),
    PseudoLegalityTest("r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", DEPTH),
    PseudoLegalityTest("r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", DEPTH),
    PseudoLegalityTest("1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", DEPTH),
    PseudoLegalityTest("2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", DEPTH),
    PseudoLegalityTest("r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", DEPTH),
    PseudoLegalityTest("8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", DEPTH),
    PseudoLegalityTest("8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", DEPTH),
    PseudoLegalityTest("B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", DEPTH),
    PseudoLegalityTest("k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", DEPTH),
    PseudoLegalityTest("K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", DEPTH),
    PseudoLegalityTest("B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", DEPTH),
    PseudoLegalityTest("k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", DEPTH),
    PseudoLegalityTest("K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1", DEPTH),
    PseudoLegalityTest("7k/RR6/8/8/8/8/rr6/7K w - - 0 1", DEPTH),
    PseudoLegalityTest("R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", DEPTH),
    PseudoLegalityTest("7k/RR6/8/8/8/8/rr6/7K b - - 0 1", DEPTH),
    PseudoLegalityTest("R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", DEPTH),
    PseudoLegalityTest("6kq/8/8/8/8/8/8/7K w - - 0 1", DEPTH),
    PseudoLegalityTest("6KQ/8/8/8/8/8/8/7k b - - 0 1", DEPTH),
    PseudoLegalityTest("K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", DEPTH),
    PseudoLegalityTest("6qk/8/8/8/8/8/8/7K b - - 0 1", DEPTH),
    PseudoLegalityTest("6KQ/8/8/8/8/8/8/7k b - - 0 1", DEPTH),
    PseudoLegalityTest("K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/8/8/8/K7/P7/k7 w - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/8/8/8/7K/7P/7k w - - 0 1", DEPTH),
    PseudoLegalityTest("K7/p7/k7/8/8/8/8/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("7K/7p/7k/8/8/8/8/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/8/8/8/K7/P7/k7 b - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/8/8/8/7K/7P/7k b - - 0 1", DEPTH),
    PseudoLegalityTest("K7/p7/k7/8/8/8/8/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("7K/7p/7k/8/8/8/8/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", DEPTH),
    PseudoLegalityTest("4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/7k/7p/7P/7K/8/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/k7/p7/P7/K7/8/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/3p4/8/3P4/8/8/7K w - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/7k/7p/7P/7K/8/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/k7/p7/P7/K7/8/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/3p4/8/3P4/8/8/7K b - - 0 1", DEPTH),
    PseudoLegalityTest("7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", DEPTH),
    PseudoLegalityTest("7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/8/7p/6P1/8/8/K7 w - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/7p/8/8/6P1/8/K7 w - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/8/6p1/7P/8/8/K7 w - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/6p1/8/8/7P/8/K7 w - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/8/3p4/4p3/8/8/7K w - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/3p4/8/8/4P3/8/7K w - - 0 1", DEPTH),
    PseudoLegalityTest("7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", DEPTH),
    PseudoLegalityTest("7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/8/7p/6P1/8/8/K7 b - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/7p/8/8/6P1/8/K7 b - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/8/6p1/7P/8/8/K7 b - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/6p1/8/8/7P/8/K7 b - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/8/3p4/4p3/8/8/7K b - - 0 1", DEPTH),
    PseudoLegalityTest("k7/8/3p4/8/8/4P3/8/7K b - - 0 1", DEPTH),
    PseudoLegalityTest("7k/8/8/p7/1P6/8/8/7K w - - 0 1", DEPTH),
    PseudoLegalityTest("7k/8/p7/8/8/1P6/8/7K w - - 0 1", DEPTH),
    PseudoLegalityTest("7k/8/8/1p6/P7/8/8/7K w - - 0 1", DEPTH),
    PseudoLegalityTest("7k/8/1p6/8/8/P7/8/7K w - - 0 1", DEPTH),
    PseudoLegalityTest("k7/7p/8/8/8/8/6P1/K7 w - - 0 1", DEPTH),
    PseudoLegalityTest("k7/6p1/8/8/8/8/7P/K7 w - - 0 1", DEPTH),
    PseudoLegalityTest("3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", DEPTH),
    PseudoLegalityTest("7k/8/8/p7/1P6/8/8/7K b - - 0 1", DEPTH),
    PseudoLegalityTest("7k/8/p7/8/8/1P6/8/7K b - - 0 1", DEPTH),
    PseudoLegalityTest("7k/8/8/1p6/P7/8/8/7K b - - 0 1", DEPTH),
    PseudoLegalityTest("7k/8/1p6/8/8/P7/8/7K b - - 0 1", DEPTH),
    PseudoLegalityTest("k7/7p/8/8/8/8/6P1/K7 b - - 0 1", DEPTH),
    PseudoLegalityTest("k7/6p1/8/8/8/8/7P/K7 b - - 0 1", DEPTH),
    PseudoLegalityTest("3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", DEPTH),
    PseudoLegalityTest("8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", DEPTH),
    PseudoLegalityTest("8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", DEPTH),
    PseudoLegalityTest("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", DEPTH),
    PseudoLegalityTest("8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", DEPTH),
    PseudoLegalityTest("8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", DEPTH),
    PseudoLegalityTest("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", DEPTH),
    PseudoLegalityTest("r1b1r3/1pp2p1p/3p4/2b2Pk1/p1PPp1Pn/P6R/1P2BP1P/R1B1K3 b Q d3 0 22", DEPTH),
};

#undef DEPTH

}