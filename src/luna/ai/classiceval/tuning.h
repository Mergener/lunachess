#ifndef LUNA_AI_CLASSICEVAL_TUNING_H
#define LUNA_AI_CLASSICEVAL_TUNING_H

#include <istream>

#include "classicevaluator.h"

namespace lunachess::ai {

struct TuningSettings {
    /** The mask of parameters to tune */
    HCEParameterMask parametersMask = HCEPM_ALL;

    int nThreads = 1;
};

struct TuningSamplePosition {
    Position position;
    int expectedScoreMP; // White POV

    inline TuningSamplePosition(const Position& pos, ChessResult res)
        : position(pos), expectedScoreMP(res) {}

    inline TuningSamplePosition(Position&& pos, ChessResult res)
        : position(std::move(pos)), expectedScoreMP(res) {}
};

/**
 * Given an input stream that reads a CSV-formatted source, generates and returns a vector
 * of TuningSamplePositions extracted using the stream.
 *
 * The source CSV is expected to have the following format:
 * <Position Fen>,<Expected clamped evaluation>
 *
 * By expected clamped evaluation, we expect a floating point value between 0 and 1, where
 * values closer to zero mean win/better for black, values closer to 1 mean win/better for white
 * and 0.5 balanced and/or drawish.
 *
 * Example:
 * rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1,0.51
 * rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1,0.96
 */
std::vector<TuningSamplePosition> fetchSamplePositionsFromCSV(std::istream& stream);

HCEWeightTable tune(const HCEWeightTable& tbl,
                    const std::vector<TuningSamplePosition>& samplePositions,
                    const TuningSettings& settings);

}

#endif // LUNA_AI_CLASSICEVAL_TUNING_H