#include "texel.h"

#include <future>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "classicevaluator.h"
#include "../../chessgame.h"
#include "../search.h"

namespace lunachess::ai {

struct TuningContext {
    std::shared_ptr<ClassicEvaluator> versionA;
    std::shared_ptr<ClassicEvaluator> versionB;
    std::unordered_map<std::string, int> fensAndEvals;
    TimeControl timeControl;
};

using GamePositions = std::vector<std::pair<std::string, int>>;

static GamePositions simulate(TuningContext& ctx,
                              std::shared_ptr<ClassicEvaluator> a,
                              std::shared_ptr<ClassicEvaluator> b) {
    GamePositions ret;

    AlphaBetaSearcher searchers[] = {AlphaBetaSearcher(a), AlphaBetaSearcher(b)};

    PlayerFunc playerFunc[2];
    for (int i = 0; i < 2; ++i) {
        playerFunc[i] = [&searchers, &ret, i](const Position &pos, TimeControl ours, TimeControl theirs) {
            SearchSettings settings;
            settings.ourTimeControl = ours;
            settings.theirTimeControl = theirs;
            SearchResults res = searchers[i].search(pos, settings);

            if (pos.getColorToMove() == CL_WHITE) {
                std::cout << "White: " << res.bestMove.toAlgebraic(pos) << " (" << res.bestScore / 10 << " cp)" << std::endl;
            }
            else {
                std::cout << "Black: " << res.bestMove.toAlgebraic(pos) << " (" << -res.bestScore / 10 << " cp)" << std::endl;
            }


            if (res.bestScore < FORCED_MATE_THRESHOLD) {
                ret.emplace_back(pos.toFen(), res.bestScore);
            }

            return res.bestMove;
        };
    }

    ChessGame game;

    playGame(game, playerFunc[0], playerFunc[1]);

    std::cout << "Finished game:\n" << game.toPgn() << std::endl;

    return ret;
}

static void playGames(TuningContext& ctx,
                      int nGames) {
    std::vector<std::future<GamePositions>> gameFutures;
    for (int i = 0; i < nGames; ++i) {
        bool random = utils::randomBool();
        gameFutures.emplace_back(std::async(std::launch::async, [&ctx, random]() {
            return simulate(ctx,
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

    playGames(ctx, 4);

}

}