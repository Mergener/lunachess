#ifndef LUNA_AI_HCEWEIGHTS_H
#define LUNA_AI_HCEWEIGHTS_H

#include <cstring>

#include <nlohmann/json.hpp>

#include "../../pst.h"

namespace lunachess::ai {

/**
 * Defines the value of each piece for determining the GPF of a position.
 */
inline constexpr i32 GPF_PIECE_VALUE_TABLE[] = {
        0, 1, 3, 3, 5, 10, 0
};

/**
 * The GPF of the standard opening position.
 */
inline constexpr i32 OPENING_GPF =
        GPF_PIECE_VALUE_TABLE[PT_KNIGHT] * 2 * 2 +
        GPF_PIECE_VALUE_TABLE[PT_BISHOP] * 2 * 2 +
        GPF_PIECE_VALUE_TABLE[PT_ROOK] * 2 * 2 +
        GPF_PIECE_VALUE_TABLE[PT_QUEEN] * 1 * 2;

struct HCEWeight {
    /** The middlegame weight. */
    i32 mg;

    /** The endgame weight. */
    i32 eg;

    /** Interpolates the middlegame and endgame weights based on a given GPF. */
    inline i32 get(i32 gamePhaseFactor) const {
        return (mg * gamePhaseFactor) / OPENING_GPF + (eg * (OPENING_GPF - gamePhaseFactor)) / OPENING_GPF;
    }

    inline HCEWeight(i32 mg, i32 eg)
            : mg(mg), eg(eg) {}

    HCEWeight() = default;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HCEWeight, mg, eg);

/**
 * Weight table used by the HCE to define position scores.
 */
struct HCEWeightTable {
    inline HCEWeightTable() {
        std::memset(this, 0, sizeof(*this));
    }

    std::array<HCEWeight, PT_COUNT> material;
    std::array<HCEWeight, 8> knightMobilityScore;
    std::array<HCEWeight, 15> bishopMobilityScore;
    std::array<HCEWeight, 7> rookHorizontalMobilityScore;
    std::array<HCEWeight, 7> rookVerticalMobilityScore;
    std::array<HCEWeight, 5> passedPawnScore;

    std::array<PieceSquareTable, 4> pawnPstsMg;
    PieceSquareTable pawnPstEg;
    PieceSquareTable kingPstMg;
    PieceSquareTable kingPstEg;

    PieceSquareTable knightPstMg;
    PieceSquareTable knightPstEg;
    PieceSquareTable bishopPstMg;
    PieceSquareTable bishopPstEg;
    PieceSquareTable rookPstMg;
    PieceSquareTable rookPstEg;
    PieceSquareTable queenPstMg;
    PieceSquareTable queenPstEg;

    HCEWeight knightOutpostScore;
    HCEWeight blockingPawnsScore;
    HCEWeight backwardPawnScore;
    HCEWeight isolatedPawnScore;
    HCEWeight kingPawnDistanceScore;
    HCEWeight bishopPairScore;
    HCEWeight rookOnOpenFile;
    HCEWeight rookBehindPasser;

    std::array<HCEWeight, PT_COUNT> checkPowerScore;
    HCEWeight queenTouchPowerScore;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HCEWeightTable, material, knightMobilityScore,
                                   bishopMobilityScore, rookHorizontalMobilityScore,
                                   rookVerticalMobilityScore, pawnPstsMg, pawnPstEg,
                                   knightPstMg, knightPstEg, bishopPstMg, bishopPstEg,
                                   rookPstMg, rookPstEg,
                                   queenPstMg, queenPstEg,
                                   kingPstMg, kingPstEg, kingPawnDistanceScore,
                                   knightOutpostScore, blockingPawnsScore,
                                   backwardPawnScore, passedPawnScore,
                                   bishopPairScore, isolatedPawnScore,
                                   rookOnOpenFile, rookBehindPasser,
                                   checkPowerScore, queenTouchPowerScore);

const HCEWeightTable* getDefaultHCEWeights();
void initializeDefaultHCEWeights();

}

#endif // LUNA_AI_HCEWEIGHTS_H
