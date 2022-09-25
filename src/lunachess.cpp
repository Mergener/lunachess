#include "lunachess.h"

#include <filesystem>

#include "ai/classic/aibitboards.h"
#include "ai/classic/hotmap.h"
#include "ai/classic/classicevaluator.h"
#include "ai/aimovefactory.h"

namespace lunachess {

static bool s_Initialized = false;

void initializeEverything() {
    // Prevent multiple setup calls
    if (s_Initialized) {
        return;
    }
    s_Initialized = true;

    endgame::initialize();

    // Initialize core functionalities
    zobrist::initialize();
    bbs::initialize();
    initializeDistances();

    // Initialize AI functionalities
    ai::initializeAIBitboards();
    ai::Hotmap::initializeHotmaps();
    ai::ClassicEvaluator::initialize();
    ai::AIMoveFactory::initialize();
}

}