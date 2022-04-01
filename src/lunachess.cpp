#include "lunachess.h"

#include <filesystem>

#include "ai/aibitboards.h"
#include "ai/hotmap.h"
#include "ai/basicevaluator.h"

namespace lunachess {

static bool s_Initialized = false;

static void install() {
    namespace fs = std::filesystem;

    fs::create_directories("data/scores");
    fs::create_directories("data/trainings");
}

void initialize() {
    if (s_Initialized) {
        return;
    }
    s_Initialized = true;

    install();

    zobrist::initialize();
    bbs::initialize();
    ai::initializeAIBitboards();
    ai::Hotmap::initializeHotmaps();
    ai::BasicEvaluator::initialize();
}

}