#include <lunachess.h>

#include <popl/popl.h>
#include <incbin/incbin.h>

#include <algorithm>
#include <random>
#include <string>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <memory>
#include <atomic>

namespace fs = std::filesystem;
using namespace lunachess;
using namespace lunachess::ai;

/** If a parameter priority is set to this value, it will be skipped. */
constexpr int PRIO_SKIP = -99999;

struct Settings {
    fs::path tunerDataPath;
    fs::path outPath;
    std::optional<fs::path> baseWeightsPath = std::nullopt;
    int threads  = 1;
    double k     = 0.120;
    bool quiesce = false;
    int repeat   = 0;
    int step     = 2;
    size_t maxPos = 1000000000;

    std::unordered_map<std::string, int> paramPriorities;
};

struct DataEntry {
    Position position;
    double   expectedScore;

    inline DataEntry(Position pos, double expectedScore)
        : position(std::move(pos)), expectedScore(expectedScore) {}
};

struct InputData {
    std::vector<DataEntry> entries;
};

INCTXT(_DefaultParamPriorities, PRIORITIES_FILE);

static int quiescenceSearch(Position& pos,
                            Move* pv,
                            int alpha = INT_MIN,
                            int beta = INT_MAX) {
    Color c = pos.getColorToMove();
    int standPat =
            100  * (pos.getBitboard(WHITE_PAWN).count()   - pos.getBitboard(BLACK_PAWN).count()) +
            320  * (pos.getBitboard(WHITE_KNIGHT).count() - pos.getBitboard(BLACK_KNIGHT).count()) +
            320  * (pos.getBitboard(WHITE_BISHOP).count() - pos.getBitboard(BLACK_BISHOP).count()) +
            520  * (pos.getBitboard(WHITE_ROOK).count()   - pos.getBitboard(BLACK_ROOK).count()) +
            1050 * (pos.getBitboard(WHITE_QUEEN).count()  - pos.getBitboard(BLACK_QUEEN).count());
    if (c == CL_BLACK) {
        standPat = -standPat;
    }
    if (standPat >= beta) {
        return beta;
    }
    if (standPat > alpha) {
        alpha = standPat;
        *(pv + 1) = MOVE_INVALID;
    }

    MoveList moves;
    movegen::generate<MTM_NOISY>(pos, moves);

    for (Move move: moves) {
        pos.makeMove(move);
        int score = -quiescenceSearch(pos, pv + 1, -beta, -alpha);
        pos.undoMove();

        if (score >= beta) {
            return beta;
        }

        if (score > alpha) {
            alpha = score;
            *pv   = move;
        }
    }

    return alpha;
}

static void quiescePosition(Position& pos) {
    std::array<Move, 128> pv;
    std::fill(pv.begin(), pv.end(), MOVE_INVALID);
    quiescenceSearch(pos, pv.data());

    for (Move move: pv) {
        if (move == MOVE_INVALID) {
            break;
        }

        pos.makeMove(move);
    }
}

static InputData parseData(fs::path dataFilePath, size_t maxPositions) {
    try {
        std::cout << "Parsing data from " << dataFilePath << std::endl;
        std::ifstream stream(dataFilePath);
        stream.exceptions(std::ios_base::badbit);

        InputData inputData;

        std::string line;
        size_t nPositions = 0;
        while (std::getline(stream, line) && nPositions < maxPositions) {
            std::vector<std::string_view> tokens;
            strutils::split(line, tokens, ",");

            std::string_view fen     = tokens[0];
            std::string_view evalStr = tokens[1];

            double eval  = std::stod(std::string(evalStr));
            Position pos = Position::fromFen(fen).value();

            inputData.entries.emplace_back(std::move(pos), eval);
            nPositions++;
        }

        std::cout << "Succesfully parsed data from " << dataFilePath << std::endl;

        return inputData;
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing data file at " << dataFilePath << ": " << e.what();
        std::exit(EXIT_FAILURE);
    }
}

static nlohmann::json loadWeightsJson(fs::path weightsJsonFilePath) {
    std::cout << "Deserializing custom weights from " << weightsJsonFilePath << std::endl;
    std::string str = utils::readFromFile(weightsJsonFilePath);
    nlohmann::json weightsJson = nlohmann::json::parse(str);
    return weightsJson;
}

static void quiesceDataPositions(InputData& inputData, int threads) {
    std::cout << "Quiescing positions...";
    std::mutex quiescedCounterMutex;
    int nQuiesced = 0;
    {
        ThreadPool threadPool(threads);
        auto chunks = utils::splitIntoChunks(inputData.entries, threads);
        for (auto chunk: chunks) {
            threadPool.enqueue([chunk, &nQuiesced, &inputData, &quiescedCounterMutex]() {
                for (int i = chunk.firstIdx; i <= chunk.lastIdx; ++i) {
                    auto& entry = inputData.entries[i];
                    quiescePosition(entry.position);

                    {
                        std::unique_lock lock(quiescedCounterMutex);
                        if ((nQuiesced % 25000) == 0) {
                            std::cout << nQuiesced << " positions quiesced..." << std::endl;
                        }
                        nQuiesced++;
                    }
                }
            });
        }
    }
    std::cout << nQuiesced << " positions quiesced." << std::endl;
}

static double sigmoid(double x, double k) {
    return 1 / (1 + std::pow(10, -k * x / 400));
}

static double squaredErrorSum(const HCEWeightTable& weights,
                              const InputData& id, int startIdx, int endIdx,
                              double k) {
    double totalError = 0;

    HandCraftedEvaluator hce(&weights);

    for (int i = startIdx; i <= endIdx; ++i) {
        const DataEntry& entry = id.entries[i];

        hce.setPosition(entry.position);
        int scoreMp = hce.evaluate();
        if (entry.position.getColorToMove() == CL_BLACK) {
            scoreMp = -scoreMp;
        }

        double score = sigmoid(double(scoreMp), k);
        double error = entry.expectedScore - score;

        totalError += error * error;
    }

    return totalError;
}

static double computeMSE(ThreadPool& threadPool,
                         const HCEWeightTable& weights,
                         const InputData& inputData,
                         double k) {
    double sum = 0;
    auto chunks = utils::splitIntoChunks(inputData.entries, inputData.entries.size() / 1000);
    std::vector<std::future<double>> partialErrors;
    for (auto chunk: chunks) {
        partialErrors.emplace_back(threadPool.submit([&inputData, &weights, chunk, k]() {
            return squaredErrorSum(weights, inputData, chunk.firstIdx, chunk.lastIdx, k);
        }));
    }

    for (auto& err: partialErrors) {
        sum += err.get();
    }
    return sum / static_cast<double>(inputData.entries.size());
}

static std::tuple<int, double> tune(const Settings& settings,
                                    const InputData& inputData,
                                    nlohmann::json flatWeightsJson,
                                    ThreadPool& threadPool,
                                    std::string parameter,
                                    int step,
                                    double lowestError = INFINITY) {
    constexpr int MAX_BAD_ITERATIONS = 4;
    constexpr double MIN_ERR_DELTA   = 0.00000001;

    int badIts    = 0;
    int value     = flatWeightsJson[parameter];
    int bestValue = value;

    while (badIts < MAX_BAD_ITERATIONS) {
        // Tune the parameter
        value += step;

        // Compute the avgError in multiple threads
        flatWeightsJson[parameter] = value;

        auto unflattened = flatWeightsJson.unflatten();
        HCEWeightTable weights(unflattened);
        double mse = computeMSE(threadPool, weights,
                                inputData,
                                settings.k);

        double delta = lowestError - mse;
        if (delta >= MIN_ERR_DELTA) {
            bestValue   = value;
            lowestError = mse;
            badIts      = 0;

            std::cout << "Good iteration -- best value " << bestValue << "(err " << lowestError << ")" << std::endl;
        }
        else {
            badIts++;
        }
    }

    return std::make_tuple(bestValue, lowestError);
}

static std::tuple<double, double> tuneK(const Settings& settings,
                                        const InputData& inputData,
                                        nlohmann::json flatWeightsJson,
                                        ThreadPool& threadPool,
                                        double step,
                                        double lowestError = INFINITY) {
    constexpr int MAX_BAD_ITERATIONS = 4;
    constexpr double MIN_ERR_DELTA   = 0.00000001;

    int badIts    = 0;
    double k      = settings.k;
    double bestK  = k;

    auto unflattened = flatWeightsJson.unflatten();
    HCEWeightTable weights(unflattened);

    while (badIts < MAX_BAD_ITERATIONS) {
        k += step;
        double mse = computeMSE(threadPool, weights,
                                inputData,
                                k);

        double delta = lowestError - mse;
        if (delta >= MIN_ERR_DELTA) {
            bestK       = k;
            lowestError = mse;
            badIts      = 0;
        }
        else {
            badIts++;
        }
    }

    return std::make_tuple(bestK, lowestError);
}

static void computeK(Settings& settings,
                    const InputData& inputData,
                    nlohmann::json flatWeightsJson) {
    std::cout << "Adjusting K... (initial guess: " << settings.k << ")" << std::endl;

    ThreadPool threadPool(settings.threads);
    auto [upK, upError]     = tuneK(settings, inputData, flatWeightsJson, threadPool, 0.01);
    auto [downK, downError] = tuneK(settings, inputData, flatWeightsJson, threadPool, -0.01);

    if (upError < downError) {
        settings.k = upK;
    }
    else {
        settings.k = downK;
    }
    std::cout << "K = " << settings.k << std::endl;
}

static int tuneParameter(const Settings& settings,
                         const InputData& inputData,
                         nlohmann::json flatWeightsJson,
                         std::string parameter) {
    auto paramJsonVal = flatWeightsJson[parameter];
    if (!paramJsonVal.is_number()) {
        throw std::runtime_error("Unsupported parameter type.");
    }
    int initialValue = paramJsonVal;
    std::cout << "Tuning parameter " << parameter << std::endl;

    ThreadPool threadPool(settings.threads);
    double lowestErr = computeMSE(threadPool, flatWeightsJson.unflatten(),
                                  inputData,
                                  settings.k);

    // We tune in "both directions" since we don't know whether the best value for
    // the parameter is higher or lower than the current one.
    auto [upValue, upError] = tune(settings,
                                                   inputData,
                                                   flatWeightsJson,
                                                   threadPool, parameter,
                                                   settings.step,
                                                   lowestErr);

    auto [downValue, downError] = tune(settings,
                                                  inputData,
                                                  flatWeightsJson,
                                                  threadPool,
                                                  parameter,
                                                  -settings.step,
                                                  lowestErr);

    if (lowestErr < upError && lowestErr < downError) {
        // Our value was already tuned
        return initialValue;
    }

    // Figure out whether we want to go up or down.
    int bestValue;
    if (downError < upError) {
        bestValue = downValue;
        lowestErr = downError;
    }
    else {
        bestValue = upValue;
        lowestErr = upError;
    }

    std::cout << "Done tuning parameter " << parameter << ": "
              << initialValue << " -> " << bestValue
              << " (err " << std::setprecision(6) << lowestErr << ")"
              << std::endl;

    return bestValue;
}

static void tuneEvaluator(Settings& settings) {
    std::cout << "Initializing tuning process" << std::endl;

    InputData inputData = parseData(settings.tunerDataPath, settings.maxPos);
    if (settings.quiesce) {
        quiesceDataPositions(inputData, settings.threads);
    }

    nlohmann::json weightsJSON = settings.baseWeightsPath.has_value()
        ? loadWeightsJson(settings.baseWeightsPath.value())
        : nlohmann::json(*getDefaultHCEWeights());

    // Flattening the weights allows us to get each field in the JSON
    // to be a single integer parameter. We can unflatten this later on
    // to create a weights table object.
    nlohmann::json flatWeights = weightsJSON.flatten();

//     Add all parameters.

    std::vector<std::string> parameters;
    std::cout << "Registering parameters..." << std::endl;
    for (const auto& item: flatWeights.items()) {
        const auto& key = item.key();

        // We're going to use each parameter priority later on when we sort them.
        // Also, we need to skip parameters that have been marked with PRIO_SKIP.
        auto it = settings.paramPriorities.find(key);
        if (it != settings.paramPriorities.end()) {
            int priority = it->second;
            if (priority <= PRIO_SKIP) {
                std::cout << "Skipping parameter " << key << std::endl;
                continue;
            }
        }
        else {
            // By default, set priority to 0
            settings.paramPriorities[key] = 0;
        }

        parameters.push_back(key);
        std::cout << "Added parameter " << key << std::endl;
    }

    computeK(settings, inputData, flatWeights);

    std::cout << "Starting tuning process." << std::endl;
    int it = 0;
    do {
        int nTunedParams = 0;
        for (const auto& param: parameters) {
            // Tune each weight individually and save it on the flatWeights again.
            // By doing this we're making sure the following weights will take into consideration
            // the tuning that was done to the ones before them.
            int newValue = tuneParameter(settings, inputData, nlohmann::json(flatWeights), param);
            std::cout << ++nTunedParams << " of " << parameters.size() << " parameters tuned." << std::endl;
            flatWeights[param] = newValue;

            // Save everything whenever we tune a parameter
            nlohmann::json unflattened = flatWeights.unflatten();
            std::ofstream ofstream(settings.outPath);
            ofstream << std::setw(2) << unflattened << std::endl;
            ofstream.close();
        }

        std::cout << "Tuning finished. Saving at " << fs::absolute(settings.outPath) << std::endl;
        it++;
    } while (it <= settings.repeat);
}

static Settings processArgs(int argc, char* argv[]) {
    try {
        Settings settings;

        popl::OptionParser op("Luna's HCE tuning tool.\nUsage");

        auto optHelp = op.add<popl::Switch>("h", "help", "Explains lunatuner's usage.");

        auto optTunerData = op.add<popl::Value<std::string>>("d", "data",
                                                             "Path to the tuning dataset");

        auto optOutPath = op.add<popl::Value<std::string>>("o", "o",
                                                           "Path to the output weights JSON file.");

        auto optPrioPath = op.add<popl::Value<std::string>>("p", "p",
                                                           "Path to the parameter priorities JSON file.");

        auto optThreads = op.add<popl::Implicit<int>>("t", "threads", "Number of threads to be used.", 1);

        auto optBaseWeights = op.add<popl::Value<std::string>>("b", "base-weights",
                                                               "JSON file of base weights. If none is provided, uses the hardcoded base HCE weights.");

        auto optQuiesce = op.add<popl::Switch>("q", "quiesce",
                                               "If set, transforms positions with captures that yield positive material balance for the moving side into quiet positions before tuning.");

        auto optRepeat = op.add<popl::Implicit<int>>("r", "repeat",
                                               "If set, repeats the tuning process by the specified number of times.", 0);


        op.parse(argc, argv);

        if (optHelp->value()) {
            // Display help and exit
            std::cout << op << std::endl;
            std::exit(EXIT_SUCCESS);
        }

        settings.tunerDataPath = optTunerData->value();
        settings.outPath       = optOutPath->value();
        settings.threads       = optThreads->value();
        settings.quiesce       = optQuiesce->value();
        settings.repeat        = optRepeat->value();

        if (optBaseWeights->is_set()) {
            settings.baseWeightsPath = optBaseWeights->value();
        }


        if (optPrioPath->is_set()) {
            settings.paramPriorities = nlohmann::json::parse(utils::readFromFile(optPrioPath->value()))
                    .get<std::unordered_map<std::string, int>>();
        }
        else {
            settings.paramPriorities = nlohmann::json::parse(std::string(g_DefaultParamPrioritiesData))
                    .get<std::unordered_map<std::string, int>>();
        }

        return settings;
    }
    catch (const std::exception& e) {
        std::cerr << "Usage error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    lunachess::initializeEverything();

    Settings settings = processArgs(argc, argv);
    tuneEvaluator(settings);

    return 0;
}