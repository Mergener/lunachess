#include "lunatest.h"

#include <string>
#include <unordered_map>

#include "tests/movegen/perft.cpp"
#include "tests/classiceval/kingexposure.cpp"
#include "tests/endgame.cpp"

namespace lunachess::tests {

std::unordered_map<std::string, std::vector<TestCase>> testGroups;

void createTests() {
    testGroups = {
        { "king-exposure", kingExposureTests }
        //{ "passers", passerAndChainTests },
        //{ "color-complex", colorComplexTests },
        //{ "perft",   perftTests },
        //{ "endgame", endgameTests },
    };
}

}