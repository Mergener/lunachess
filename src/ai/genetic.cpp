#include "genetic.h"

#include <algorithm>
#include <random>
#include <future>
#include <fstream>
#include <atomic>
#include <filesystem>

#include "../serial.h"
#include "../strutils.h"
#include "../movegen.h"

namespace lunachess::ai::genetic {

namespace fs = std::filesystem;

static std::mt19937_64 s_Random (std::random_device{}());

static int randomInt(int min, int max) {
    if (min == max) {
        return min;
    }

    int delta = max - min;
    int ret = (s_Random() % delta) + min;
    return ret;
}

static int mutate(int val, int chance, int minPct = -6, int maxPct = 6, int minAbs = -3, int maxAbs = 3) {
    // Check chance
    int rnd = s_Random() % 100;
    if (rnd >= chance) {
        // Do nothing
        return val;
    }

    // Randomize numbers
    int abs = randomInt(minAbs, maxAbs);
    int pct = randomInt(minPct, maxPct);

    val = val * (100 + pct) / 100 + abs;
    return val;
}

void mutate(BasicEvaluator& eval, const MutationSettings& settings) {
    mutate(eval.getMiddlegameScores(), settings);
    eval.getMiddlegameScores().materialScore[PT_PAWN] = 1000; // Keep pawns at the correct unit
    mutate(eval.getEndgameScores(), settings);
}

void mutate(ScoreTable& scores, const MutationSettings& settings) {
    int mutChance = settings.mutationChancePct;

    scores.mobilityScore = std::max(0, mutate(scores.mobilityScore, mutChance, 0, 0, -2, 2));
    scores.bishopPairScore = std::max(0, mutate(scores.bishopPairScore, mutChance));
    scores.outpostScore = std::max(0, mutate(scores.outpostScore, mutChance));
    scores.pawnShieldScore = mutate(scores.pawnShieldScore, mutChance);
    scores.kingOnOpenFileScore = std::max(0, mutate(scores.kingOnOpenFileScore, mutChance));
    scores.kingNearOpenFileScore = std::max(0, mutate(scores.kingNearOpenFileScore, mutChance));
    scores.kingOnSemiOpenFileScore = std::max(0, mutate(scores.kingOnSemiOpenFileScore, mutChance));
    scores.kingNearSemiOpenFileScore = std::max(0, mutate(scores.kingNearSemiOpenFileScore, mutChance));
    scores.doublePawnScore = std::min(0, mutate(scores.doublePawnScore, mutChance));
    scores.pawnChainScore = std::max(0, mutate(scores.pawnChainScore, mutChance));
    scores.passerPercentBonus = std::max(0, mutate(scores.passerPercentBonus, mutChance));
    scores.outsidePasserPercentBonus = std::max(0, mutate(scores.outsidePasserPercentBonus, mutChance));
    scores.goodComplexScore = std::max(0, mutate(scores.goodComplexScore, mutChance));

    //for (int& t: scores.tropismScore) {
    //    t = mutate(t, mutChance);
    //}

    for (int& x: scores.xrayScores) {
        x = mutate(x, mutChance, -0, 0, -2, 2);
    }

    for (int& t: scores.materialScore) {
        t = mutate(t, mutChance, 0, 0, -150, 150);
    }

    for (auto &t: scores.kingHotmap) {
        t = mutate(t, mutChance, 0, 0, -15, 15);
    }

    for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
        // Get hotmap group
        auto& hmg = scores.hotmapGroups[pt - 1];

        for (Square s = 0; s < 64; ++s) {
            if (s % 8 >= 4) {
                continue;
            }

            // Find corresponding hotmap based on king's square
            auto& hm = hmg.getHotmap(s);
            for (short &t: hm) {
                t = mutate(t, mutChance, 0, 0, -15, 15);
            }
        }
    }
}

inline static constexpr int average(int a, int b) {
    return (a + b) / 2;
}

ScoreTable crossover(const ScoreTable& a, const ScoreTable& b) {
    ScoreTable ret;

    ret.mobilityScore = average(a.mobilityScore, b.mobilityScore);
    ret.bishopPairScore = average(a.bishopPairScore, b.bishopPairScore);
    ret.outpostScore = average(a.outpostScore, b.outpostScore);
    ret.pawnShieldScore = average(a.pawnShieldScore, b.pawnShieldScore);
    ret.kingOnOpenFileScore = average(a.kingOnOpenFileScore, b.kingOnOpenFileScore);
    ret.kingNearOpenFileScore = average(a.kingNearOpenFileScore, b.kingNearOpenFileScore);
    ret.kingOnSemiOpenFileScore = average(a.kingOnSemiOpenFileScore, b.kingOnSemiOpenFileScore);
    ret.kingNearSemiOpenFileScore = average(a.kingNearSemiOpenFileScore, b.kingNearSemiOpenFileScore);
    ret.doublePawnScore = average(a.doublePawnScore, b.doublePawnScore);
    ret.pawnChainScore = average(a.pawnChainScore, b.pawnChainScore);
    ret.passerPercentBonus = average(a.passerPercentBonus, b.passerPercentBonus);
    ret.outsidePasserPercentBonus = average(a.outsidePasserPercentBonus, b.outsidePasserPercentBonus);

    for (PieceType pt = PT_PAWN; pt < PT_COUNT; ++pt) {
        ret.materialScore[pt] = average(a.materialScore[pt], b.materialScore[pt]);
    }

    for (PieceType pt = PT_PAWN; pt < PT_COUNT; ++pt) {
        ret.tropismScore[pt] = average(a.tropismScore[pt], b.tropismScore[pt]);
    }

    for (PieceType pt = PT_PAWN; pt < PT_COUNT; ++pt) {
        ret.xrayScores[pt] = average(a.xrayScores[pt], b.xrayScores[pt]);
    }

    for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
        for (Square ks = 0; ks < 64; ++ks) {
            if (ks % 8 >= 4) {
                continue;
            }

            const auto& hma = a.hotmapGroups[pt - 1].getHotmap(ks);
            const auto& hmb = b.hotmapGroups[pt - 1].getHotmap(ks);

            for (Square s = 0; s < 64; ++s) {
                int aVal = hma.getValue(s, CL_BLACK);
                int bVal = hmb.getValue(s, CL_BLACK);

                ret.hotmapGroups[pt - 1].getHotmap(ks).setValue(s, CL_BLACK, average(aVal, bVal));
            }
        }
    }

    return ret;
}

BasicEvaluator crossover(const BasicEvaluator& a, const BasicEvaluator& b) {
    BasicEvaluator ret;

    ret.setMiddlegameScores(crossover(a.getMiddlegameScores(), b.getMiddlegameScores()));
    ret.setEndgameScores(crossover(a.getEndgameScores(), b.getEndgameScores()));

    return ret;
}

AlphaBetaSearcher crossover(const AlphaBetaSearcher& a, const AlphaBetaSearcher& b) {
    AlphaBetaSearcher ret;

    ret.getEvaluator() = crossover(a.getEvaluator(), b.getEvaluator());

    return ret;
}

//
// Genetic Training
//

void Training::Generation::generateAgents(int n, MutationSettings settings) {
    for (int i = 0; i < n; ++i) {
        agents.emplace_back(s_Random());
        Agent& last = agents[agents.size() - 1];

        mutate(last.getEvaluator(), settings);
    }
}

Training::Training(const TrainingSettings& settings)
    : m_Settings(settings) {

    MutationSettings agentGenSettings;
    agentGenSettings.mutationChancePct = 80;

    m_CurrGeneration.agents.emplace_back(0);
    m_CurrGeneration.generateAgents(settings.agentsPerGen - 1, agentGenSettings);

    std::cout << "Generated " << settings.agentsPerGen << " agents." << std::endl;
}

Training::Training(const fs::path& path) {
    // TO-DO
}

void Training::start() {
    if (m_CurrState != TrainingState::Idle) {
        return;
    }

    m_CurrState = TrainingState::Running;
    m_CurrMutSettings.mutationChancePct = m_Settings.initialMutRate;

    std::cout << "Starting training." << std::endl;

    while (true) {
        playGames();
        makeNewGeneration();
    }
}

static SearchResults doMoveSearch(const Position& pos, AlphaBetaSearcher& searcher, int maxTime) {
    SearchSettings searchSettings;
    searchSettings.ourTimeControl.time = maxTime;
    searchSettings.ourTimeControl.mode = TC_MOVETIME;

    SearchResults res = searcher.search(pos, searchSettings);

    return res;
}

int Training::Game::play(Agent& white, Agent& black, int movetime) {
    agentIds[CL_WHITE] = white.getId();
    agentIds[CL_BLACK] = black.getId();

    AlphaBetaSearcher searchers[CL_COUNT] = {
        white.getEvaluatorPtr(),
        black.getEvaluatorPtr()
    };

    std::cout << "Starting training game between agents " << white.getName() << " and "
              << black.getName() << "." << std::endl;

    Position pos = Position::getInitialPosition();

    MoveList ml;
    bool draw = false;

    while (true) {
        Agent& currentAgent = pos.getColorToMove() == CL_WHITE ? white : black;

        ml.clear();

        movegen::generate(pos, ml);

        if (ml.size() == 0) {
            // Checkmate/stalemate
            if (!pos.isCheck()) {
                // Stalemate
                draw = true;
            }
            break;
        }

        if (pos.isDraw()) {
            draw = true;
            break;
        }

        SearchResults res = doMoveSearch(pos, searchers[pos.getColorToMove()], movetime);

        Move bestMove;
        if (res.bestMove == MOVE_INVALID) {
            bestMove = moves[0];
        }
        else {
            bestMove = res.bestMove;
        }

        pos.makeMove(bestMove);
        moves.push_back(bestMove);

        std::cout << "Agent " << currentAgent.getName() << " played " << res.bestMove
                  << ". (pov score " << res.bestScore << " depth " << res.searchedDepth << ")" << std::endl;
    };

    int ret;

    std::cout << "Game between agents " << white.getName() << " and " << black.getName()
              << " has finished. Result: ";

    if (draw) {
        white.addDraw(CL_WHITE);
        black.addDraw(CL_BLACK);
        std::cout << "Draw." << std::endl;
        return 0;
    }
    else if (pos.getColorToMove() == CL_WHITE) {
        white.addLoss(CL_WHITE);
        black.addWin(CL_BLACK);
        std::cout << "Agent "  << black.getName() << " (black) wins!" << std::endl;
        ret = -1;
    }
    else {
        white.addWin(CL_WHITE);
        black.addLoss(CL_BLACK);
        std::cout << "Agent "  << white.getName() << " (white) wins!" << std::endl;
        ret = 1;
    }
    return ret;
}

void Training::playRound(Agent &a, Agent &b) {
    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
        Game game;
        Agent *white;
        Agent *black;

        if (c == CL_WHITE) {
            white = &a;
            black = &b;
        } else {
            white = &b;
            black = &a;
        }

        game.play(*white, *black, m_Settings.movetime);

        m_CurrGeneration.games.emplace_back(game);
    }
}

void Training::playGames() {
    std::atomic_int runningThreads = 0; // The number of threads that can be spawned

    for (int i = 0; i < m_CurrGeneration.agents.size() - 1; ++i) {
        for (int j = i + 1; j < m_CurrGeneration.agents.size(); ++j) {
            save(m_Settings.savePath);
            // Procedure that runs a game between two agents
            auto proc = [this, i, j, &runningThreads]() {
                Agent& a = m_CurrGeneration.agents[i];
                Agent& b = m_CurrGeneration.agents[j];

                try {
                    playRound(a, b);
                }
                catch (const std::exception&) {
                    runningThreads--;
                    return;
                }
                runningThreads--;
            };

            if (m_Settings.maxThreads <= 1) {
                // Single threaded, run on main thread.
                proc();
                continue;
            }

            // Multi threaded
            while (runningThreads >= m_Settings.maxThreads); // Wait for a thread to be available

            // A thread is finally available
            runningThreads++;
            std::thread(proc).detach();
        }
    }

    // Wait remaining threads
    while (runningThreads > 0);
}

void Training::reproduceAgents() {
    for (int i = 0; i < m_Settings.selectNumber - 1; ++i) {
        for (int j = 0; j < m_Settings.selectNumber; ++j) {
            if (m_CurrGeneration.agents.size() == m_Settings.agentsPerGen) {
                // We generated enough agents
                break;
            }

            // Get parents
            Agent& a = m_CurrGeneration.agents[i];
            Agent& b = m_CurrGeneration.agents[j];

            // Generate new agent
            m_CurrGeneration.agents.emplace_back(s_Random());
            Agent& agent = m_CurrGeneration.agents[m_CurrGeneration.agents.size() - 1];

            // New agent is a crossover of parents + mutations
            agent.getEvaluator() = crossover(a.getEvaluator(), b.getEvaluator());
            mutate(agent.getEvaluator(), m_CurrMutSettings);
        }
    }

    for (Agent& ag : m_CurrGeneration.agents) {
        ag.resetWinsAndLosses();
    }
}

void Training::performSelection() {
    std::sort(m_CurrGeneration.agents.begin(), m_CurrGeneration.agents.end(),
              [](const Agent& a, const Agent& b) {
                  return a.getFitness() > b.getFitness();
              });
    m_CurrGeneration.agents.erase(m_CurrGeneration.agents.begin() + m_Settings.selectNumber,
                                  m_CurrGeneration.agents.end());
}

void Training::makeNewGeneration() {
    m_CurrGenLock.lock();
    // Save current generation
    m_PrevGenerations.push_back(m_CurrGeneration);

    performSelection();
    reproduceAgents();

    // Should more agents be generated to fill in gaps?
    int remaining = m_Settings.agentsPerGen - m_CurrGeneration.agents.size();
    if (remaining > 0) {
        // Yes, generate them
        m_CurrGeneration.generateAgents(remaining, MutationSettings());
    }

    // Update some stuff
    m_CurrMutSettings.mutationChancePct -= m_Settings.mutRatePerGen;
    m_CurrGenLock.unlock();

    // Clear list of games
    m_CurrGeneration.games.clear();

    std::thread([this]() { save(m_Settings.savePath); } ).detach();
}

void Training::save(const fs::path& where) {
    fs::create_directory(where);

    std::cout << "Saving..." << std::endl;

    fs::path genPath = where / "generations";
    fs::create_directory(genPath);

    for (int i = 0; i < m_PrevGenerations.size(); ++i) {
        const auto& gen = m_PrevGenerations[i];
        gen.save(genPath / strutils::toString(i));
    }
    m_CurrGenLock.lock();
    m_CurrGeneration.save(genPath / strutils::toString(m_PrevGenerations.size()));
    m_CurrGenLock.unlock();

    std::cout << "Finished saving." << std::endl;
}

void Training::Generation::save(const fs::path& path) const {
    fs::create_directory(path);

    // Save all agents
    fs::path agentsPath = path / "agents";
    fs::create_directory(agentsPath);
    for (const auto& a : agents) {
        a.save(agentsPath / a.getName());
    }

    // Save all games
    fs::path gamesPath = path / "games";
    fs::create_directory(gamesPath);
    for (int i = 0; i < games.size(); ++i) {
        const auto& g = games[i];

        g.save(gamesPath);
    }
}

void Training::Agent::save(const fs::path& path) const {
    fs::create_directory(path);

    std::ofstream mgStream(path / "mg.scores");
    mgStream << getEvaluator().getMiddlegameScores() << std::endl;
    mgStream.close();

    std::ofstream egStream(path / "eg.scores");
    egStream << getEvaluator().getEndgameScores() << std::endl;
    egStream.close();

    std::ofstream profileStream(path / "profile");

    SerialObject obj;
    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
        obj[std::string("wins_") + getColorName(c)] = m_Wins[c];
        obj[std::string("losses_") + getColorName(c)] = m_Losses[c];
        obj[std::string("draws_") + getColorName(c)] = m_Draws[c];
    }
    obj["fitness"] = getFitness();
    obj["id"] = getId();

    obj["parent_a"] = m_Parents[0];
    obj["parent_b"] = m_Parents[1];

    obj["generation"] = m_Gen;

    profileStream << obj;
}

void Training::Game::save(const fs::path& path) const {
    fs::path finalPath;

    // Generate filename
    std::stringstream fileNameStream;
    fileNameStream << std::hex << agentIds[0]
                   << "_vs_"
                   << std::hex << agentIds[1]
                   << ".pgn";

    std::string fileName = fileNameStream.str();

    finalPath = path / fileName;
    if (std::filesystem::exists(finalPath)) {
        return;
    }

    std::ofstream stream(finalPath);

    for (Move move : moves) {
        stream << move << std::endl;
    }
}

}