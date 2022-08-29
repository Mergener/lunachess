#include "lunachess.h"

#include <filesystem>

#include "ai/aibitboards.h"
#include "ai/hotmap.h"
#include "ai/basicevaluator.h"
#include "ai/aimovefactory.h"

namespace lunachess {

static bool s_Initialized = false;

static void install() {
    namespace fs = std::filesystem;

    fs::create_directories("data/scores");
    fs::create_directories("data/trainings");
}

void initializeEverything() {
    // Prevent multiple setup calls
    if (s_Initialized) {
        return;
    }
    s_Initialized = true;

    // Create necessary directories
    install();

    // Initialize core functionalities
    zobrist::initialize();
    bbs::initialize();

    // Initialize AI functionalities
    ai::initializeAIBitboards();
    ai::Hotmap::initializeHotmaps();
    ai::BasicEvaluator::initialize();
    ai::AIMoveFactory::initialize();
}

}