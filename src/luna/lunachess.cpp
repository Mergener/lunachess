#include "lunachess.h"

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
    ai::initializeDefaultHCEWeights();
    ai::initializeSearchParameters();
}

}