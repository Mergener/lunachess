#include <iostream>
#include <ctime>
#include <iomanip>
#include <thread>

#include "lunachess.h"
#include "ai/genetic.h"
#include "strutils.h"

using namespace lunachess;
using namespace lunachess::ai;

static genetic::Training* s_Training = nullptr;
static std::string s_TrainingName;

static int trainingMain(int movetime) {
    namespace gen = ai::genetic;

    // Generate training name;
    time_t now = time(0);
    tm* ltm = localtime(&now);
    std::stringstream nameStream;
    nameStream << std::setw(2) << std::setfill('0')
               << ltm->tm_hour << "-"
               << std::setw(2) << std::setfill('0')
               << ltm->tm_min << "-"
               << std::setw(2) << std::setfill('0')
               << ltm->tm_sec << "_"
               << ltm->tm_year + 1900 << "-"
               << std::setw(2) << std::setfill('0')
               << ltm->tm_mon + 1 << "-"
               << std::setw(2) << std::setfill('0')
               << ltm->tm_mday;
    s_TrainingName = nameStream.str();

    std::atexit([]() {
        if (s_Training == nullptr) {
            return;
        }

        s_Training->save("data/trainings/" + s_TrainingName);
    });

    gen::TrainingSettings settings;
    settings.agentsPerGen = 16;
    settings.selectNumber = 6;
    settings.savePath = "data/trainings/" + s_TrainingName;
    settings.movetime = movetime;
    settings.maxThreads = std::thread::hardware_concurrency() - 1;
    settings.initialMutRate = 6;
    settings.mutRatePerGen = -3;
    settings.minMutRate = 1;

    s_Training = new gen::Training(settings);
    s_Training->start();

    return 0;
}

int main(int argc, char* argv[]) {
    lunachess::initialize();

    std::ios_base::sync_with_stdio(false);
    std::cin.tie();

    if (argc > 1 && std::string(argv[1]) == "train") {
        int movetime = 1000;

        if (argc > 2) {
            if (!strutils::tryParseInteger(argv[2], movetime)) {
                movetime = 250;
            }
        }

        return trainingMain(movetime);
    }

    return uciMain();
}