#include "lunatest.h"

#include <string>
#include <unordered_map>

#include "tests/movegen/perft.cpp"
#include "tests/classiceval/colorcomplex.cpp"
#include "tests/classiceval/mobility.cpp"
#include "tests/classiceval/nearkingatks.cpp"
#include "tests/classiceval/passerandchains.cpp"
#include "tests/endgame.cpp"

namespace lunachess::tests {

std::unordered_map<std::string, std::vector<TestCase>> testGroups;

void createTests() {
    testGroups = {
        { "passers", passerAndChainTests },
        { "near-king-attacks", nearKingAttacksTests },
        { "color-complex", colorComplexTests },
        { "mobility", mobilityTests },
        { "perft",   perftTests },
        { "endgame", endgameTests },
    };
}

}