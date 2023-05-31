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

namespace lunachess::ai {

/**
 * Type of parameter that is evaluated by the HCE.
 */
enum HCEParameter {
    HCEP_MATERIAL,
    HCEP_MOBILITY,
    HCEP_PASSED_PAWNS,
    HCEP_BACKWARD_PAWNS,
    HCEP_PLACEMENT,
    HCEP_KNIGHT_OUTPOSTS,
    HCEP_BLOCKING_PAWNS,
    HCEP_ISOLATED_PAWNS,
    HCEP_ENDGAME_THEORY,

    HCEP_PARAM_COUNT,
};

using HCEParameterMask = ui64;

static constexpr HCEParameterMask HCEPM_ALL = ~0;

/**
 * Defines the value of each piece for determining the GPF of a position.
 */
inline constexpr int GPF_PIECE_VALUE_TABLE[] = {
        0, 1, 3, 3, 5, 10, 0
};

/**
 * The GPF of the standard opening position.
 */
inline constexpr int OPENING_GPF =
        GPF_PIECE_VALUE_TABLE[PT_KNIGHT] * 2 * 2 +
        GPF_PIECE_VALUE_TABLE[PT_BISHOP] * 2 * 2 +
        GPF_PIECE_VALUE_TABLE[PT_ROOK] * 2 * 2 +
        GPF_PIECE_VALUE_TABLE[PT_QUEEN] * 1 * 2;

struct HCEWeight {
    /** The middlegame weight. */
    int mg;

    /** The endgame weight. */
    int eg;

    /** Interpolates the middlegame and endgame weights based on a given GPF. */
    inline int get(int gamePhaseFactor) const {
        return (mg * gamePhaseFactor) / OPENING_GPF + (eg * (OPENING_GPF - gamePhaseFactor)) / OPENING_GPF;
    }

    inline HCEWeight(int mg, int eg)
        : mg(mg), eg(eg) {}
};

extern PieceSquareTable g_DEFAULT_PAWN_PST_MG;
extern PieceSquareTable g_DEFAULT_PAWN_PST_EG;

extern PieceSquareTable g_DEFAULT_KING_PST_MG;
extern PieceSquareTable g_DEFAULT_KING_PST_EG;

/**
 * Weight table used by the HCE to define position scores.
 * Initialized to Luna's default values.
 */
struct HCEWeightTable {
    std::array<HCEWeight, PT_COUNT> material = {
        HCEWeight(0, 0),        // PT_NONE
        HCEWeight(1000, 1000),  // PT_PAWN
        HCEWeight(3100, 3500),  // PT_KNIGHT
        HCEWeight(3200, 3800),  // PT_BISHOP
        HCEWeight(5100, 5800),  // PT_ROOK
        HCEWeight(9100, 10000), // PT_QUEEN
        HCEWeight(0, 0),        // PT_KING
    };

    std::array<HCEWeight, 8> knightMobilityScore = {
        HCEWeight(0, 0),
        HCEWeight(0, 0),
        HCEWeight(50, 20),
        HCEWeight(75, 30),
        HCEWeight(100, 40),
        HCEWeight(125, 50),
        HCEWeight(150, 60),
        HCEWeight(175, 70),
    };

    std::array<HCEWeight, 10> bishopMobilityScore = {
        HCEWeight(-100, -50),
        HCEWeight(-50, -25),
        HCEWeight(0, 0),
        HCEWeight(50, 20),
        HCEWeight(75, 30),
        HCEWeight(100, 40),
        HCEWeight(125, 50),
        HCEWeight(150, 60),
        HCEWeight(175, 70),
        HCEWeight(200, 80),
    };

    PieceSquareTable mgPSTs[PT_COUNT] = {
        {},
        g_DEFAULT_PAWN_PST_MG,
        {},
        {},
        {},
        {},
        g_DEFAULT_KING_PST_MG,
    };
    PieceSquareTable egPSTs[PT_COUNT] = {
        {},
        g_DEFAULT_PAWN_PST_EG,
        {},
        {},
        {},
        {},
        g_DEFAULT_KING_PST_EG,
    };

    HCEWeight knightOutpostScore = { 150, 100 };

    HCEWeight blockingPawnsScore = { -100, -200 };

    HCEWeight backwardPawnScore = { -75, -150 };

    HCEWeight isolatedPawnScore = { -100, -200 };

    HCEWeight passedPawnScore = { 100, 250 };

    std::array<HCEWeight, 7> rookHorizontalMobilityScore = {
        HCEWeight(0, 0),
        HCEWeight(0, 0),
        HCEWeight(40, 0),
        HCEWeight(40, 40),
        HCEWeight(40, 40),
        HCEWeight(40, 40),
        HCEWeight(40, 40),
    };

    std::array<HCEWeight, 7> rookVerticalMobilityScore = {
        HCEWeight(0, 0),
        HCEWeight(0, 0),
        HCEWeight(50, 40),
        HCEWeight(50, 40),
        HCEWeight(100, 40),
        HCEWeight(125, 40),
        HCEWeight(125, 40),
    };
};

/**
 * A hand-crafted evaluator that uses human domain knowledge to evaluate positions.
 *
 * Several evaluation parameters are evaluated, including material count, king safety, pawn structure...
 * Parameters can be toggled on or off based on the specified parameter mask.
 * By default, all parameters are set to ON.
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

    inline HandCraftedEvaluator(HCEParameterMask mask = HCEPM_ALL)
        : m_ParamMask(mask) {
    }

private:
    HCEWeightTable m_Weights;
    HCEParameterMask m_ParamMask;

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
    int getPassedPawnsScore(int gpf, Color c) const;
    int getBackwardPawnsScore(int gpf, Color c) const;

public:
    inline const HCEWeightTable& getWeights() const {
        return m_Weights;
    }

    inline void setWeights(const HCEWeightTable& weights) {
        m_Weights = weights;
    }

    inline HCEParameterMask getParameterMask() const {
        return m_ParamMask;
    }

    inline void setParameterMask(HCEParameterMask mask) {
        m_ParamMask = mask;
    }

    inline void toggleParameter(HCEParameter param, bool on) {
        if (on) {
            m_ParamMask |= BIT(param);
        }
        else {
            m_ParamMask &= ~BIT(param);
        }
    }
};
}

#endif // LUNA_AI_CLASSIC_EVALUATOR_H