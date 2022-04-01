#include "perft.h"

#include <map>

#include "movegen.h"

namespace lunachess {

int promotions = 0;
int enPassants = 0;

template <bool PSEUDO_LEGAL, bool LOG>
static ui64 perftInternal(Position& pos, int depth) {
    MoveList moves;

    ui64 n;
    n = movegen::generate<MTM_ALL, PSEUDO_LEGAL>(pos, moves);

    if (depth <= 1) {
        if constexpr (LOG) {
            for (auto m: moves) {
                std::cout << m << ": 1" << std::endl;
            }
            std::cout << (pos.isCheck() ? "check" : "not check") << std::endl;
        }
        return n;
    }

    depth--;
    ui64 ret = 0;

    for (auto m : moves) {
        pos.makeMove(m);
        ui64 count = perftInternal<PSEUDO_LEGAL, false>(pos, depth);
        ret += count;
        pos.undoMove();

        if constexpr (LOG) {
            std::cout << m << ": " << count << std::endl;
        }
    }

    if constexpr (LOG) {
        std::cout << std::endl;
    }

    return ret;
}

ui64 perft(const Position& pos, int depth, bool pseudoLegal) {
    Position repl = pos;

    ui64 ret;

    if (pseudoLegal) {
        ret = perftInternal<true, true>(repl, depth);
    }
    else {
        ret = perftInternal<false, true>(repl, depth);
    }

    return ret;
}

} // lunachess