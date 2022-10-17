#ifndef LUNA_AI_CLASSIC_EVALUATOR_H
#define LUNA_AI_CLASSIC_EVALUATOR_H

#include <iostream>
#include <array>
#include <functional>

#include <nlohmann/json.hpp>

#include "../evaluator.h"
#include "evalscores.h"

#include "../../endgame.h"

namespace lunachess::ai {

class ClassicEvaluator : public Evaluator {
public:
    int getDrawScore() const override;
    int evaluate(const Position& pos) const override;
    int evaluateShallow(const Position& pos) const override;

    int getGamePhaseFactor(const Position& pos) const;

    inline ScoreTable& getMiddlegameScores() { return m_MgScores; }
    inline const ScoreTable& getMiddlegameScores() const { return m_MgScores; }
    inline void setMiddlegameScores(const ScoreTable& scores) {
        m_MgScores = scores;
    }

    inline ScoreTable& getEndgameScores() { return m_EgScores; }
    inline const ScoreTable& getEndgameScores() const { return m_EgScores; }
    inline void setEndgameScores(const ScoreTable& scores) {
        m_EgScores = scores;
    }

    ClassicEvaluator();
    inline ClassicEvaluator(const ScoreTable& mg, const ScoreTable& eg)
        : m_MgScores(mg), m_EgScores(eg) {}

    int evaluateMaterial(const Position& pos, Color c, int gpf) const;
    int evaluateBlockingPawns(const Position& pos, Color c, int gpf) const;
    int evaluateChainsAndPassers(const Position& pos, Color c, int gpf) const;
    int evaluatePawnComplex(const Position& pos, Color c, int gpf) const;
    int evaluateHotmaps(const Position& pos, Color c, int gpf) const;
    int evaluateOutposts(const Position& pos, Color c, int gpf) const;
    int evaluateAttacks(const Position& pos, Color c, int gpf) const;
    int evaluateBishopPair(const Position& pos, Color c, int gpf) const;
    int evaluateClassic(const Position& pos) const;
    int evaluateEndgame(const Position& pos, EndgameData egData) const;
    int evaluateKPK(const Position& pos, Color lhs) const;
    int evaluateKBNK(const Position& pos, Color lhs) const;
    int evaluateNearKingAttacks(const Position& pos, Color c, int gpf) const;
    int evaluatePawnShield(const Position& pos, Color c, int gpf) const;
    int evaluateKingExposure(const Position& pos, Color c, int gpf) const;

    // Pawn structure
    static Bitboard getPassedPawns(const Position& pos, Color c);
    static Bitboard getChainPawns(const Position& pos, Color c);

private:
    ScoreTable m_MgScores;
    ScoreTable m_EgScores;

    int evaluateCornerMateEndgame(const Position& pos, EndgameData eg) const;

    static void generateNewMgTable();
    static void generateNewEgTable();
};

std::istream& operator>>(std::istream& stream, ScoreTable& scores);
std::ostream& operator<<(std::ostream& stream, const ScoreTable& scores);

}

#endif // LUNA_AI_CLASSIC_EVALUATOR_H