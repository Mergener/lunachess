#include "staticanalysis.h"

namespace lunachess::tests {

struct OutpostTest {
    std::string fen;
    Piece piece;
    Bitboard expectedBB;

    OutpostTest(std::string fen, Piece piece, Bitboard expectedBB)
        : fen(fen), piece(piece), expectedBB(expectedBB) {}

    void operator()() {
        Position pos = Position::fromFen(fen).value();
        Bitboard result = staticanalysis::getPieceOutposts(pos, piece);

        LUNA_ASSERT(result == expectedBB,
                    "Expected:\n" << expectedBB << "\n\nGot:\n" << result);
    }
};


std::vector<TestCase> outpostTests = {
    OutpostTest("8/8/6k1/3N4/2P5/8/6K1/8 w - - 0 1", WHITE_KNIGHT, 0x800000000),
    OutpostTest("8/2p5/6k1/3N4/2P5/8/6K1/8 w - - 0 1", WHITE_KNIGHT, 0),
    OutpostTest("8/8/2p3k1/3N4/2P5/8/6K1/8 w - - 0 1", WHITE_KNIGHT, 0),
    OutpostTest("8/8/6k1/2pN4/2P5/8/6K1/8 w - - 0 1", WHITE_KNIGHT, 0x800000000),
    OutpostTest("8/8/6k1/3N4/2P5/2p5/6K1/8 w - - 0 1", WHITE_KNIGHT, 0x800000000),
    OutpostTest("8/8/6k1/3N4/2P5/2p5/6K1/8 w - - 0 1", WHITE_KNIGHT, 0x800000000),
};

}