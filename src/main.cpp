#include <iostream>
#include <thread>

#include <ctime>

#include "lunachess.h"

#include "ai/neural/neuralgenetic.h"

using namespace lunachess;

void doGeneticTraining() {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);

    std::stringstream nameStream;
    nameStream << (now->tm_year + 1900) << '-'
               << (now->tm_mon + 1) << '-'
               <<  now->tm_mday << '_'
               << now->tm_hour << '-'
               << now->tm_min << '-'
               << now->tm_sec << '-';

    lunachess::ai::neural::GeneticTraining training;
    lunachess::ai::neural::GeneticTrainingSettings settings;
    settings.baseSavePath = std::filesystem::path("trainings") / nameStream.str();

    training.run();
}

int main(int argc, char* argv[]) {
    try {
        lunachess::initializeEverything();

        std::ios_base::sync_with_stdio(false);
        std::cin.tie();
        std::cout << std::boolalpha;

        doGeneticTraining();
        return 0;

        return uciMain();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw e;
    }
}