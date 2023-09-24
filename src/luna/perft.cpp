#include "perft.h"

#include <map>

#include "movegen.h"
#include "ai/movecursor.h"

namespace lunachess {

template <bool PSEUDO_LEGAL, bool ALG_NOTATION, bool LOG>
static ui64 perftInternal(Position& pos, int depth) {
    MoveList moves;

    ui64 n;
    n = movegen::generate<MTM_ALL, PTM_ALL, PSEUDO_LEGAL>(pos, moves);

    if (depth <= 1) {
        if constexpr (LOG) {
            for (auto m: moves) {
                if constexpr (ALG_NOTATION) {
                    std::cout << m.toAlgebraic(pos) << ": 1" << std::endl;
                }
                else {
                    std::cout << m << ": 1"  << std::endl;
                }
            }
            std::cout << (pos.isCheck() ? "check" : "not check") << std::endl;
        }
        return n;
    }

    depth--;
    ui64 ret = 0;

    for (auto m : moves) {
        pos.makeMove(m);
        ui64 count = perftInternal<PSEUDO_LEGAL, ALG_NOTATION, false>(pos, depth);
        ret += count;
        pos.undoMove();

        if constexpr (LOG) {
            if constexpr (ALG_NOTATION) {
                std::cout << m.toAlgebraic(pos) << ": " << count << std::endl;
            }
            else {
                std::cout << m << ": " << count << std::endl;
            }
        }
    }

    if constexpr (LOG) {
        std::cout << std::endl;
    }

    return ret;
}

ui64 perft(const Position& pos, int depth, bool log, bool pseudoLegal, bool algNotation) {
    Position repl = pos;

    ui64 ret;

    if (log) {
        if (algNotation) {
            if (pseudoLegal) {
                ret = perftInternal<true, true, true>(repl, depth);
            } else {
                ret = perftInternal<false, true, true>(repl, depth);
            }
        } else {
            if (pseudoLegal) {
                ret = perftInternal<true, false, true>(repl, depth);
            } else {
                ret = perftInternal<false, false, true>(repl, depth);
            }
        }
    }
    else {
        if (pseudoLegal) {
            ret = perftInternal<true, true, false>(repl, depth);
        } else {
            ret = perftInternal<false, true, false>(repl, depth);
        }
    }

    return ret;
}

} // lunachess