#ifndef LUNA_AI_CLASSIC_EVALUATOR_H
#define LUNA_AI_CLASSIC_EVALUATOR_H

#include <iostream>
#include <array>
#include <functional>

#include <nlohmann/json.hpp>

#include "../evaluator.h"
#include "../../pst.h"
#include "../../bits.h"
#include "../../utils.h"

#include "../../endgame.h"

#include "hceweights.h"

namespace lunachess::ai {

/**
 * A hand-crafted evaluator that uses human domain knowledge to evaluate positions.
 *
 * Several evaluation parameters are evaluated, including material count, king safety, pawn structure...
 */
class HandCraftedEvaluator : public Evaluator {
public:
    /**
     * Returns the game phase factor (GPF) of a position.
     * A GPF closer to zero indicates a game that is closer to the endgame (fewer pieces).
     * In standard chess games, the GPF of the initial position will be equal to OPENING_GPF.
     *
     * This value is used to interpolate middlegame and endgame scores.
     */
    int getGamePhaseFactor() const;

    int evaluate() const override;

    inline int getDrawScore() const override { return 0; }

    inline HandCraftedEvaluator(const HCEWeightTable* weights = getDefaultHCEWeights())
        : m_Weights(weights) {
    }

private:
    const HCEWeightTable* m_Weights;

    int evaluateClassic(const Position& pos) const;
    int evaluateEndgame(const Position& pos, EndgameData egData) const;
    int evaluateKPK(const Position& pos, Color lhs) const;
    int evaluateKBNK(const Position& pos, Color lhs) const;

    int getMaterialScore(int gpf, Color c) const;
    int getMobilityScore(int gpf, Color c) const;
    int getPlacementScore(int gpf, Color c) const;
    int getKnightOutpostScore(int gpf, Color c) const;
    int getBlockingPawnsScore(int gpf, Color c) const;
    int getIsolatedPawnsScore(int gpf, Color c) const;
    int getPassedPawnsScore(int gpf, Color c, Bitboard passers) const;
    int getBackwardPawnsScore(int gpf, Color c) const;
    int getKingPawnDistanceScore(int gpf, Color c) const;
    int getBishopPairScore(int gpf, Color c) const;
    int getKingAttackScore(int gpf, Color us) const;
    int getRooksScore(int gpf, Color c, Bitboard passers) const;

public:
    inline const HCEWeightTable& getWeights() const {
        return *m_Weights;
    }

    inline void setWeights(const HCEWeightTable* weights) {
        m_Weights = weights;
    }
};
}

#endif // LUNA_AI_CLASSIC_EVALUATOR_H