#ifndef LUNA_AI_CLASSICEVAL_TUNING_H
#define LUNA_AI_CLASSICEVAL_TUNING_H

#include <istream>

#include "classicevaluator.h"

namespace lunachess::ai {

struct TuningSettings {
    /** The mask of parameters to tune */
    HCEParameterMask parametersMask = HCEPM_ALL;
};

struct TuningSamplePosition {
    Position position;
    ChessResult finalResult; // White POV

    inline TuningSamplePosition(const Position& pos, ChessResult res)
        : position(pos), finalResult(res) {}

    inline TuningSamplePosition(Position&& pos, ChessResult res)
        : position(std::move(pos)), finalResult(res) {}
};

std::vector<TuningSamplePosition> fetchSamplePositionsFromCSV(std::istream& stream);

HCEWeightTable tune(const HCEWeightTable& tbl,
                    const std::vector<TuningSamplePosition>& samplePositions,
                    const TuningSettings& settings);

}

#endif // LUNA_AI_CLASSICEVAL_TUNING_H