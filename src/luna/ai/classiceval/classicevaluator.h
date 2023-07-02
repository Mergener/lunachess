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
    HCEP_KING_EXPOSURE,
    HCEP_KING_PAWN_DISTANCE,
    HCEP_PAWN_SHIELD,
    HCEP_TROPISM,
    HCEP_BISHOP_PAIR,
    HCEP_KING_ATTACK,
    HCEP_DIMINISHING_MATERIAL_GAINS,

    HCEP_PARAM_COUNT,
};

using HCEParameterMask = ui64;

static constexpr HCEParameterMask HCEPM_ALL = ~0;

/**
 * Defines the value of each piece for determining the GPF of a position.
 */
inline constexpr int GPF_PIECE_VALUE_TABLE[] = {
        0, 1, 3, 3, 6, 12, 0
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

    HCEWeight() = default;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HCEWeight, mg, eg);

extern PieceSquareTable g_DEFAULT_PAWN_PST_MG;
extern PieceSquareTable g_DEFAULT_PAWN_PST_EG;

extern PieceSquareTable g_DEFAULT_KING_PST_MG;
extern PieceSquareTable g_DEFAULT_KING_PST_EG;


/**
 * Weight table used by the HCE to define position scores.
 * Initialized to Luna's default values.
 */
struct HCEWeightTable {

    HCEWeightTable() = default;

    std::array<HCEWeight, PT_COUNT> material = {
        HCEWeight(0, 0),        // PT_NONE
        HCEWeight(1000, 1000),  // PT_PAWN
        HCEWeight(3200, 3600),  // PT_KNIGHT
        HCEWeight(3500, 4000),  // PT_BISHOP
        HCEWeight(5100, 5600),  // PT_ROOK
        HCEWeight(9100, 9600), // PT_QUEEN
        HCEWeight(0, 0),        // PT_KING
    };

    std::array<HCEWeight, 8> knightMobilityScore = {
        HCEWeight(-380, -330),
        HCEWeight(-250, -230),
        HCEWeight(-120, -130),
        HCEWeight(0, -30),
        HCEWeight(120, 70),
        HCEWeight(250, 170),
        HCEWeight(310, 220),
        HCEWeight(380, 270),
    };

    std::array<HCEWeight, 15> bishopMobilityScore = {
        HCEWeight(-250, -30),
        HCEWeight(-110, -16),
        HCEWeight(30, -2),
        HCEWeight(170, 12),
        HCEWeight(310, 26),
        HCEWeight(450, 40),
        HCEWeight(570, 52),
        HCEWeight(650, 60),
        HCEWeight(710, 65),
        HCEWeight(740, 69),
        HCEWeight(760, 71),
        HCEWeight(780, 73),
        HCEWeight(790, 74),
        HCEWeight(800, 75),
        HCEWeight(810, 76),
    };

    std::array<HCEWeight, 7> rookHorizontalMobilityScore = {
        HCEWeight(0, 0),
        HCEWeight(0, 0),
        HCEWeight(40, 0),
        HCEWeight(100, 100),
        HCEWeight(100, 100),
        HCEWeight(100, 100),
        HCEWeight(100, 100),
    };

    std::array<HCEWeight, 7> rookVerticalMobilityScore = {
        HCEWeight(-100, -200),
        HCEWeight(-50, -100),
        HCEWeight(0, 0),
        HCEWeight(200, 250),
        HCEWeight(300, 400),
        HCEWeight(400, 500),
        HCEWeight(500, 600),
    };

    std::array<PieceSquareTable, PT_COUNT> mgPSTs = {
        PieceSquareTable {},
        g_DEFAULT_PAWN_PST_MG,
        PieceSquareTable {},
        PieceSquareTable {},
        PieceSquareTable {},
        PieceSquareTable {},
//        PieceSquareTable {},
        g_DEFAULT_KING_PST_MG,
    };

    std::array<PieceSquareTable, PT_COUNT> egPSTs = {
        PieceSquareTable {},
        g_DEFAULT_PAWN_PST_EG,
        PieceSquareTable {},
        PieceSquareTable {},
        PieceSquareTable {},
        PieceSquareTable {},
        //PieceSquareTable {},
        g_DEFAULT_KING_PST_EG,
    };

    HCEWeight knightOutpostScore = { 300, 200 };

    HCEWeight blockingPawnsScore = { -50, -120 };

    HCEWeight backwardPawnScore = { -75, -150 };

    HCEWeight isolatedPawnScore = { -50, -120 };

    /**
     * Score for passed pawns. Indexes are the number of steps the pawn is away
     * from promotion minus 1.
     * Ex: a white pawn on b4 is 4 steps away from promotion (b4->b5->b6->b7->b8=?),
     * so the passer bonus for the pawn is passedPawnScore[3].
     */
    std::array<HCEWeight, 5> passedPawnScore = {
        HCEWeight(100, 500),
        HCEWeight(100, 500),
        HCEWeight(100, 500),
        HCEWeight(100, 500),
        HCEWeight(100, 500),
    };

    HCEWeight queenExposureScore = { -360, -180 };
    HCEWeight bishopExposureScore = { -220, 0 };
    HCEWeight rookExposureScore = { -200, 0 };
    HCEWeight knightExposureScore = { -60, 0 };

    HCEWeight kingPawnDistanceScore = { 0, -70 };

    std::array<HCEWeight, 4> pawnShieldScore = {
        HCEWeight(-120, 0),
        HCEWeight(0, 0),
        HCEWeight(150, 0),
        HCEWeight(220, 0),
    };

    std::array<HCEWeight, 8> knightTropismScore = {
        HCEWeight(175, 90),
        HCEWeight(150, 75),
        HCEWeight(125, 60),
        HCEWeight(100, 45),
        HCEWeight(75, 30),
        HCEWeight(50, 15),
        HCEWeight(25, 0),
        HCEWeight(0, 0),
    };

    std::array<HCEWeight, 8> bishopTropismScore = {
        HCEWeight(175, 90),
        HCEWeight(150, 75),
        HCEWeight(125, 60),
        HCEWeight(100, 45),
        HCEWeight(75, 30),
        HCEWeight(50, 15),
        HCEWeight(25, 0),
        HCEWeight(0, 0),
    };

    std::array<HCEWeight, 8> rookTropismScore = {
        HCEWeight(175, 90),
        HCEWeight(150, 75),
        HCEWeight(125, 60),
        HCEWeight(100, 45),
        HCEWeight(75, 30),
        HCEWeight(50, 15),
        HCEWeight(25, 0),
        HCEWeight(0, 0),
    };

    std::array<HCEWeight, 8> queenTropismScore = {
        HCEWeight(275, 180),
        HCEWeight(275, 180),
        HCEWeight(275, 180),
        HCEWeight(240, 150),
        HCEWeight(220, 120),
        HCEWeight(150, 80),
        HCEWeight(60, 20),
        HCEWeight(0, 0),
    };

    HCEWeight bishopPairScore = { 150, 260 };

    /**
     * Attack powers:
     * p: 1
     * n: 3
     * b: 3
     * r: 6
     * q: 12
     */
    std::array<int, 60> kingAttackScore = {
        10,
        20,
        32,
        44,
        58,
        70,
        84,
        100,
        120,
        150,
        190,
        230,
        270,
        320,
        370,
        420,
        480,
        540,
        610,
        700,
        800,
        900,
        1050,
        1200,
        1400,
        1600,
        1900,
        2400,
        3000,
        3300,
        3500,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
        3600,
    };
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HCEWeightTable, material, knightMobilityScore,
                                   bishopMobilityScore, rookHorizontalMobilityScore,
                                   rookVerticalMobilityScore, mgPSTs, egPSTs,
                                   knightOutpostScore, blockingPawnsScore,
                                   backwardPawnScore, passedPawnScore, queenExposureScore,
                                   bishopExposureScore, rookExposureScore, knightExposureScore,
                                   kingPawnDistanceScore, pawnShieldScore, knightTropismScore,
                                   bishopTropismScore, rookTropismScore, queenTropismScore,
                                   bishopPairScore, kingAttackScore, isolatedPawnScore);
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
    int getKingExposureScore(int gpf, Color c) const;
    int getKingPawnDistanceScore(int gpf, Color c) const;
    int getPawnShieldScore(int gpf, Color c) const;
    int getTropismScore(int gpf, Color c) const;
    int getBishopPairScore(int gpf, Color c) const;
    int getKingAttackScore(int gpf, Color c) const;

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