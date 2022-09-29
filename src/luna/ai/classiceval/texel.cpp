#include "texel.h"

#include <future>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

#include "classicevaluator.h"
#include "../../chessgame.h"
#include "../search.h"
#include "../../openingbook.h"

namespace lunachess::ai {

namespace fs = std::filesystem;

struct TuningContext {
    std::shared_ptr<ClassicEvaluator> versionA;
    std::shared_ptr<ClassicEvaluator> versionB;
    std::unordered_map<std::string, int> fensAndEvals;
    TimeControl timeControl;
    OpeningBook openingBook;
    fs::path savePath;
};

using GamePositions = std::vector<std::pair<std::string, int>>;

static GamePositions simulate(TuningContext& ctx,
                              int gameId,
                              std::shared_ptr<ClassicEvaluator> a,
                              std::shared_ptr<ClassicEvaluator> b) {
    GamePositions ret;

    AlphaBetaSearcher searchers[] = {AlphaBetaSearcher(a), AlphaBetaSearcher(b)};

    PlayerFunc playerFunc[2];
    for (int i = 0; i < 2; ++i) {
        playerFunc[i] = [&searchers, &ret, i](const Position &pos, TimeControl ours, TimeControl theirs) {
            const OpeningBook& book = OpeningBook::getDefault();


            Move bestMove = book.getRandomMoveForPosition(pos);

            if (bestMove == MOVE_INVALID) {
                SearchSettings settings;
                settings.ourTimeControl = ours;
                settings.theirTimeControl = theirs;

                SearchResults res = searchers[i].search(pos, settings);

                bestMove = res.bestMove;
                if (res.bestScore < FORCED_MATE_THRESHOLD) {
                    ret.emplace_back(pos.toFen(), searchers[i].quiesce(pos));
                }
            }

            return bestMove;
        };
    }

    ChessGame game;

    PlayGameArgs gameArgs;
    gameArgs.timeControl[0] = ctx.timeControl;
    gameArgs.timeControl[1] = ctx.timeControl;
    playGame(game, playerFunc[0], playerFunc[1], gameArgs);

    std::ofstream gameStream(ctx.savePath / (utils::toString(gameId) + ".pgn"));
    gameStream << game.toPgn();
    gameStream.close();

    return ret;
}

static void playGames(TuningContext& ctx,
                      int nGames) {
    std::vector<std::future<GamePositions>> gameFutures;
    for (int i = 0; i < nGames; ++i) {
        bool random = utils::randomBool();
        gameFutures.emplace_back(std::async(std::launch::async, [&ctx, random, i]() {
            return simulate(ctx, i + 1,
                            random ? ctx.versionA : ctx.versionB,
                            random ? ctx.versionB : ctx.versionA);
        }));
    }

    int nFinished = 0;
    for (auto& f: gameFutures) {
        auto vec = f.get();
        for (const auto& pair: vec) {
            ctx.fensAndEvals[pair.first] = pair.second;
        }
        nFinished++;
        std::cout << nFinished << " games finished." << std::endl;
    }
}

void runTexelTuning() {
    TuningContext ctx;

    std::string path;
    std::cout << "Select a path: ";
    std::cin >> path;

    ctx.savePath = path;
    fs::create_directories(ctx.savePath);
    ctx.versionA = std::make_shared<ClassicEvaluator>();
    ctx.versionB = std::make_shared<ClassicEvaluator>();
    ctx.timeControl = TimeControl(1000, 50, TC_FISCHER);

    playGames(ctx, 64000);

}

}