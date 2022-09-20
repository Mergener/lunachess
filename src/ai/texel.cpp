#include "texel.h"

#include <future>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "classicevaluator.h"
#include "search.h"

namespace lunachess::ai {

struct TuningContext {
    std::shared_ptr<ClassicEvaluator> versionA;
    std::shared_ptr<ClassicEvaluator> versionB;
    std::unordered_map<std::string, int> fensAndEvals;
    TimeControl timeControl;
};

using GamePositions = std::vector<std::pair<std::string, int>>;

static GamePositions playGame(TuningContext& ctx,
                              std::shared_ptr<ClassicEvaluator> a,
                              std::shared_ptr<ClassicEvaluator> b) {
    GamePositions ret;
    Position pos = Position::getInitialPosition();

    AlphaBetaSearcher searchers[] = { AlphaBetaSearcher(a), AlphaBetaSearcher(b) };

    // Configure time controls
    SearchSettings settings;
    settings.ourTimeControl = ctx.timeControl;
    settings.theirTimeControl = ctx.timeControl;

    Color curr = pos.getColorToMove();
    bool flagged = false;
    ChessResult gameRes;

    // Play the game until it ends, either by clock or board
    while ((gameRes = pos.getResult(CL_WHITE, !flagged)) == RES_UNFINISHED) {
        SearchResults results = searchers[curr].search(pos, settings);

        settings.ourTimeControl.time -= results.getSearchTime();
        if (settings.ourTimeControl.time <= 0) {
            flagged = true;
            continue;
        }
        settings.ourTimeControl.time += settings.ourTimeControl.increment;
        std::swap(settings.ourTimeControl, settings.theirTimeControl);

        pos.makeMove(results.bestMove);

        if (results.bestScore >= FORCED_MATE_THRESHOLD) {
            continue;
        }

        ret.emplace_back(pos.toFen(), results.bestScore);
    }

    return ret;
}

static void playGames(TuningContext& ctx,
                      int nGames) {
    std::vector<std::future<GamePositions>> gameFutures;
    for (int i = 0; i < nGames; ++i) {
        bool random = utils::randomBool();
        gameFutures.emplace_back(std::async(std::launch::async, [&ctx, random]() {
            return playGame(ctx,
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

}