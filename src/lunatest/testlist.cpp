#include "lunatest.h"

#include <string>
#include <unordered_map>

#include "tests/movegen/perft.cpp"
#include "tests/movegen/pseudolegal.cpp"
#include "tests/endgame.cpp"
#include "tests/staticanalysis/outposts.cpp"
#include "tests/staticanalysis/backwardpawns.cpp"
#include "tests/staticanalysis/blockingpawns.cpp"
#include "tests/staticanalysis/connectedpawns.cpp"
#include "tests/staticanalysis/passedpawns.cpp"

namespace lunachess::tests {

std::unordered_map<std::string, std::vector<TestCase>> testGroups;

void createTests() {
    testGroups = {
        { "perft",          perftTests },
        { "pseudoLegality", pseudoLegalityTests },
        { "outposts",       outpostTests },
        { "backwardPawns",  backwardPawnsTests },
        { "blockingPawns",  blockingPawnsTests },
        { "connectedPawns", connectedPawnsTests },
        { "passedPawns",    passedPawnsTests },
        { "endgame",        endgameTests },
    };
}

}