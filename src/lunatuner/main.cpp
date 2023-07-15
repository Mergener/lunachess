#include <lunachess.h>

#include <popl/popl.h>

#include <string>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <memory>
#include <atomic>

namespace fs = std::filesystem;
using namespace lunachess;
using namespace lunachess::ai;

struct Settings {
    fs::path tunerDataPath;
    fs::path outPath;
    int threads = 1;
    double k = 0.113;
    std::optional<fs::path> baseWeightsPath = std::nullopt;
};

struct DataEntry {
    Position position;
    double expectedScore;

    inline DataEntry(Position pos, double expectedScore)
        : position(std::move(pos)), expectedScore(expectedScore) {}
};

struct InputData {
    std::vector<DataEntry> entries;
};

static Settings processArgs(int argc, char* argv[]) {
    try {
        Settings settings;

        popl::OptionParser op("Luna's HCE tuning tool.\nUsage");

        auto optHelp = op.add<popl::Switch>("h", "help", "Explains lunatuner's usage.");

        auto optTunerData = op.add<popl::Value<std::string>>("d", "data",
                                                             "Path to the tuning dataset");

        auto optOutPath = op.add<popl::Value<std::string>>("o", "o",
                                                           "Path to the output weights JSON file.");

        auto optThreads = op.add<popl::Implicit<int>>("t", "threads", "Number of threads to be used.", 1);

        auto optBaseWeights = op.add<popl::Value<std::string>>("b", "base-weights",
                                                               "JSON file of base weights. If none is provided, uses the hardcoded base HCE weights.");

        op.parse(argc, argv);

        if (optHelp->value()) {
            // Display help and exit
            std::cout << op << std::endl;
            std::exit(EXIT_SUCCESS);
        }

        settings.tunerDataPath   = optTunerData->value();
        settings.outPath         = optOutPath->value();
        settings.threads         = optThreads->value();

        if (optBaseWeights->is_set()) {
            settings.baseWeightsPath = optBaseWeights->value();
        }

        return settings;
    }
    catch (const std::exception& e) {
        std::cerr << "Usage error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

static int quiesce(Position& pos,
                   Move* pv,
                   int alpha = INT_MIN,
                   int beta = INT_MAX) {
    Color c = pos.getColorToMove();
    int standPat =
            100  * (pos.getBitboard(WHITE_PAWN).count()   - pos.getBitboard(BLACK_PAWN).count()) +
            350  * (pos.getBitboard(WHITE_KNIGHT).count() - pos.getBitboard(BLACK_KNIGHT).count()) +
            360  * (pos.getBitboard(WHITE_BISHOP).count() - pos.getBitboard(BLACK_BISHOP).count()) +
            550  * (pos.getBitboard(WHITE_ROOK).count()   - pos.getBitboard(BLACK_ROOK).count()) +
            1100 * (pos.getBitboard(WHITE_QUEEN).count()  - pos.getBitboard(BLACK_QUEEN).count());
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
        int score = -quiesce(pos, pv + 1, -beta, -alpha);
        pos.undoMove();

        if (score >= beta) {
            return beta;
        }

        if (score > alpha) {
            alpha = score;
            *pv = move;
        }
    }

    return alpha;
}

static void acquiescePosition(Position& pos) {
    std::array<Move, 128> pv;
    std::fill(pv.begin(), pv.end(), MOVE_INVALID);
    quiesce(pos, pv.data());

    for (Move move: pv) {
        if (move == MOVE_INVALID) {
            break;
        }

        pos.makeMove(move);
    }
}

static InputData parseData(fs::path dataFilePath, int threads) {
    try {
        std::cout << "Parsing data from " << dataFilePath << std::endl;
        std::ifstream stream(dataFilePath);
        stream.exceptions(std::ios_base::badbit);

        InputData inputData;

        std::string line;
        while (std::getline(stream, line)) {
            std::vector<std::string_view> tokens;
            strutils::split(line, tokens, ",");

            std::string_view fen = tokens[0];
            std::string_view evalStr = tokens[1];

            double eval = std::stod(std::string(evalStr));
            Position pos = Position::fromFen(fen).value();

            inputData.entries.emplace_back(std::move(pos), eval);
        }

        std::mutex acquiescedCounterMutex;
        int nAcquiesced = 0;
        {
            ThreadPool threadPool(threads);
            auto chunks = utils::splitIntoChunks(inputData.entries, threads);
            for (auto chunk: chunks) {
                threadPool.enqueue([chunk, &nAcquiesced, &inputData, &acquiescedCounterMutex]() {
                    for (int i = chunk.firstIdx; i <= chunk.lastIdx; ++i) {
                        auto& entry = inputData.entries[i];
                        acquiescePosition(entry.position);

                        {
                            std::unique_lock lock(acquiescedCounterMutex);
                            if ((nAcquiesced % 25000) == 0) {
                                std::cout << nAcquiesced << " positions acquiesced..." << std::endl;
                            }
                            nAcquiesced++;
                        }

                    }
                });
            }
        }
        std::cout << nAcquiesced << " positions acquiesced." << std::endl;

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

static double sigmoid(double x, double k) {
    return 1 / (1 + std::pow(10, -k * x / 400));
}

static double evaluationErrorSum(nlohmann::json flatWeightsJson,
                                 const InputData& id, int startIdx, int endIdx,
                                 double k) {
    double totalError = 0;
    int n = 0;

    HCEWeightTable weights = flatWeightsJson.unflatten();
    HandCraftedEvaluator hce(&weights);

    for (int i = startIdx; i <= endIdx; ++i) {
        const DataEntry& entry = id.entries[i];

        hce.setPosition(entry.position);
        int scoreMp = hce.evaluate();
        if (entry.position.getColorToMove() == CL_BLACK) {
            scoreMp = -scoreMp;
        }

        double score = sigmoid(double(scoreMp), k);
        double error = std::abs(entry.expectedScore - score);

        totalError += error;

        n++;
    }

    return totalError;
}

static std::tuple<int, double> tune(const Settings& settings,
                                   const InputData& inputData,
                                   nlohmann::json flatWeightsJson,
                                   std::string parameter,
                                   int step) {
    constexpr int MAX_BAD_ITERATIONS = 5;
    int badIts = 0;
    int value = flatWeightsJson[parameter];
    int bestValue = value;
    double lowestError = INFINITY;

    ThreadPool threadPool(settings.threads);
    while (true) {
        double avgError = 0;

        // Tune the parameter
        value += step;

        // Compute the avgError in multiple threads
        flatWeightsJson[parameter] = value;

        auto chunks = utils::splitIntoChunks(inputData.entries, settings.threads);
        std::vector<std::future<double>> partialErrors;
        for (auto chunk: chunks) {
            partialErrors.push_back(threadPool.submit([&]() {
                return evaluationErrorSum(flatWeightsJson, inputData, chunk.firstIdx, chunk.lastIdx, settings.k);
            }));
        }

        for (auto& err: partialErrors) {
            avgError += err.get();
        }
        avgError /= inputData.entries.size();

        if (avgError < lowestError) {
            bestValue = value;
            lowestError = avgError;
            badIts = 0;
        }
        else {
            // We need to stop when we think we're starting to overtune or undertune
            // our parameter. If we get a certain minimum of "bad" iterations (iterations
            // that increased the avgError), interrupt the search.
            badIts++;
            if (badIts >= MAX_BAD_ITERATIONS) {
                break;
            }
        }
    }

    return std::make_tuple(bestValue, lowestError);
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

    // We tune in "both directions" since we don't know whether the best value for
    // the parameter is higher or lower than the current one.
    auto [upValue, upError] = tune(settings, inputData, flatWeightsJson, parameter, 1);
    auto [downValue, downError] = tune(settings, inputData, flatWeightsJson, parameter, -1);

    double lowestErr;
    int bestValue;
    if (downError < upError) {
        bestValue = downValue;
        lowestErr = downError;
    }
    else {
        bestValue = upValue;
        lowestErr = upError;
    }

    std::cout << std::setprecision(8) << "Done tuning parameter " << parameter
              << ": " << initialValue << " -> " << bestValue << " (err " << std::setprecision(6) << lowestErr
              << ")" << std::endl;

    return bestValue;
}

static void tune(const Settings& settings) {
    std::cout << "Initializing tuning process" << std::endl;

    InputData inputData = parseData(settings.tunerDataPath, settings.threads);

    nlohmann::json weightsJSON = settings.baseWeightsPath.has_value()
        ? loadWeightsJson(settings.baseWeightsPath.value())
        : nlohmann::json(*getDefaultHCEWeights());

    // Flattening the weights allows us to get each field in the JSON
    // to be a single integer parameter. We can unflatten this later on
    // to create a weights table object.
    nlohmann::json flatWeights = weightsJSON.flatten();

    int i = 0;
    for (const auto& item: flatWeights.items()) {
        // Tune each weight individually and save it on the flatWeights again.
        // By doing this we're making sure the following weights will take into consideration
        // the tuning that was done to the ones before them.
        std::string param = item.key();
        int newValue = tuneParameter(settings, inputData, flatWeights, param);
        std::cout << ++i << " of " << flatWeights.size() << " parameters tuned." << std::endl;
        flatWeights[param] = newValue;

        // Save everything whenever we tune a parameter
        nlohmann::json unflattened = flatWeights.unflatten();
        std::ofstream ofstream(settings.outPath);
        ofstream << std::setw(2) << unflattened << std::endl;
        ofstream.close();
    }

    std::cout << "Tuning finished. Saving at " << fs::absolute(settings.outPath) << std::endl;
}

int main(int argc, char* argv[]) {
    lunachess::initializeEverything();

    Settings settings = processArgs(argc, argv);
    tune(settings);

    return 0;
}