#ifndef LUNA_AI_GENETIC_H
#define LUNA_AI_GENETIC_H

#include <string>
#include <vector>
#include <cstring>
#include <filesystem>

#include "../lock.h"

#include "basicevaluator.h"
#include "search.h"

// Abbreviation for std::filesystem.
// Gets undef'd by the end of the header.
#define fs std::filesystem

namespace lunachess::ai::genetic {

struct MutationSettings {
    int mutationChancePct = 4;
};

void mutate(BasicEvaluator& eval, const MutationSettings& settings = MutationSettings());
void mutate(ScoreTable& scores, const MutationSettings& settings = MutationSettings());

ScoreTable crossover(const ScoreTable& a, const ScoreTable& b);
BasicEvaluator crossover(const BasicEvaluator& a, const BasicEvaluator& b);
AlphaBetaSearcher crossover(const AlphaBetaSearcher& a, const AlphaBetaSearcher& b);

struct TrainingSettings {
    // Genetic settings
    std::vector<BasicEvaluator> presetAgents;
    int agentsPerGen = 8;

    /** Number of agents to be selected from each generation.  */
    int selectNumber = 3;

    /** Initial mutation rate.*/
    int initialMutRate = 25;

    /** Minimum mutation rate. */
    int minMutRate = 2;

    /** Maximum mutation rate. */
    int maxMutRate = 25;

    /** How much the mutation rate should change per generation. */
    int mutRatePerGen = -1;

    // Technical settings
    int maxThreads = 1;
    fs::path savePath;

    // Game settings

    /** Time per move, in milliseconds, during training games. */
    int movetime = 1000;
};

enum class TrainingState {
    Idle,
    Running,
};

class Training {
public:
    void start();
    void stop();

    void save(const fs::path& where);

    inline TrainingState getCurrentState() const { return m_CurrState; }

    /** Creates a new training */
    Training(const TrainingSettings& settings);

    /** Loads an existing training from disk */
    Training(const fs::path& path);

private:
    TrainingSettings m_Settings;
    TrainingState m_CurrState = TrainingState::Idle;
    MutationSettings m_CurrMutSettings;

    class Agent {
    public:
        inline int getWins(Color c) const { return m_Wins[c]; }
        inline int getLosses(Color c) const { return m_Losses[c]; }
        inline int getDraws(Color c) const { return m_Draws[c]; }

        inline void addWin(Color c) { m_Wins[c]++; }
        inline void addLoss(Color c) { m_Losses[c]++; }
        inline void addDraw(Color c) { m_Draws[c]++; }

        inline BasicEvaluator& getEvaluator() { return *m_Eval; }
        inline const BasicEvaluator& getEvaluator() const { return *m_Eval; }

        inline std::shared_ptr<BasicEvaluator> getEvaluatorPtr() { return m_Eval; }

        inline ui64 getId() const { return m_Id; }

        inline void resetWinsAndLosses() {
            for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
                m_Wins[c] = 0;
                m_Losses[c] = 0;
                m_Draws[c] = 0;
            }
        }

        inline std::string getName() const {
            char name[24];
            sprintf(name, "%llx", getId());
            return std::string(name);
        }

        inline int getFitness() const {
            return m_Wins[CL_WHITE] * 4 - m_Losses[CL_WHITE] * 5 - m_Draws[CL_WHITE],
                   m_Wins[CL_BLACK] * 5 - m_Losses[CL_BLACK] * 4 + m_Draws[CL_BLACK];
        }

        inline Agent(ui64 id)
            :m_Id(id), m_Eval(std::make_shared<BasicEvaluator>()) {
            std::memset(m_Wins, 0, sizeof(m_Wins));
            std::memset(m_Losses, 0, sizeof(m_Losses));
            std::memset(m_Draws, 0, sizeof(m_Draws));
        }

        void save(const fs::path& path) const;

    private:
        ui64 m_Id;
        std::shared_ptr<BasicEvaluator> m_Eval;

        int m_Gen;
        int m_Parents[2];

        int m_Wins[CL_COUNT];
        int m_Losses[CL_COUNT];
        int m_Draws[CL_COUNT];
    };

    struct Game {
        std::vector<Move> moves;
        ui64 agentIds[CL_COUNT];

        int play(Agent& white, Agent& black, int movetime);

        void save(const fs::path& path) const;
    };

    struct Generation {
        std::vector<Agent> agents;
        std::vector<Game> games;

        void generateAgents(int n, MutationSettings settings);

        void save(const fs::path& path) const;
    };

    std::vector<Generation> m_PrevGenerations;
    Generation m_CurrGeneration;
    Lock m_CurrGenLock;

    void playGames();
    void playRound(Agent& a, Agent& b);
    void makeNewGeneration();
    void reproduceAgents();
    void performSelection();
};

}

#undef fs

#endif // LUNA_AI_GENETIC_H
