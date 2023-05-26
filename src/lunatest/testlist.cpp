#include "lunatest.h"

#include <string>
#include <unordered_map>

#include "tests/movegen/perft.cpp"
#include "tests/endgame.cpp"
#include "tests/staticanalysis/outposts.cpp"
#include "tests/staticanalysis/backwardpawns.cpp"

namespace lunachess::tests {

std::unordered_map<std::string, std::vector<TestCase>> testGroups;

void createTests() {
    testGroups = {
        //{ "passers", passerAndChainTests },
        //{ "color-complex", colorComplexTests },
        { "perft",   perftTests },
        {"outposts", outpostTests },
        {"backwardPawns", backwardPawnsTests },
        { "endgame", endgameTests },
    };
}

}