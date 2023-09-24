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
    i32 getGamePhaseFactor() const;

    i32 evaluate() const override;

    inline i32 getDrawScore() const override { return 0; }

    inline HandCraftedEvaluator(const HCEWeightTable* weights = getDefaultHCEWeights())
        : m_Weights(weights) {
    }

private:
    const HCEWeightTable* m_Weights;

    // Evaluation functions
    i32 evaluateClassic(const Position& pos, Color us) const;
    i32 evaluateEndgame(const Position& pos, EndgameData egData) const;
    i32 evaluateKPK(const Position& pos, Color lhs) const;
    i32 evaluateKBPK(const Position& pos, Color lhs) const;
    i32 evaluateKBNK(const Position& pos, Color lhs) const;

    // Evaluation features
    i32 getMaterialScore(i32 gpf, Color c) const;
    i32 getMobilityScore(i32 gpf, Color c) const;
    i32 getPlacementScore(i32 gpf, Color c) const;
    i32 getKnightOutpostScore(i32 gpf, Color c) const;
    i32 getBlockingPawnsScore(i32 gpf, Color c) const;
    i32 getIsolatedPawnsScore(i32 gpf, Color c) const;
    i32 getPassedPawnsScore(i32 gpf, Color c, Bitboard passers) const;
    i32 getBackwardPawnsScore(i32 gpf, Color c) const;
    i32 getKingPawnDistanceScore(i32 gpf, Color c) const;
    i32 getBishopPairScore(i32 gpf, Color c) const;
    i32 getKingAttackScore(i32 gpf, Color us) const;
    i32 getRooksScore(i32 gpf, Color c, Bitboard passers) const;

    // King-attack related functions
    i32 getCheckPower(i32 gpf, Color us) const;
    i32 getQueenTouchPower(i32 gpf, Color us) const;
    i32 getTropismPower(i32 gpf, Color us) const;

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