#include "tuning.h"

#include <string_view>
#include <string>

#include "../../strutils.h"
#include "../quiescevaluator.h"
#include "classicevaluator.h"

namespace lunachess::ai {

struct TuningParameter {
    std::string parameterName;
    int* ptr;
};

static constexpr double K = 2;

static double sigmoid(double x, double k) {
    return 1 / (1 + std::pow(10, -K * x / 400));
}

static double averageEvaluationError(Evaluator& eval,
                                     const std::vector<TuningSamplePosition>& posData,
                                     int kMp) {
    double resSum = 0;
    for (auto& d: posData) {
        eval.setPosition(d.position);
        int qScoreMp = eval.evaluate();
        resSum += d.expectedScoreMP / 1000.0;

        resSum -= sigmoid(qScoreMp / 1000.0, kMp / 1000.0);
    }

    return resSum / posData.size();
}

static void addParameter(std::vector<TuningParameter>& vec, std::string_view name, HCEWeight& weight) {
    vec.push_back(TuningParameter { std::string(name) + "MG", &weight.mg });
    vec.push_back(TuningParameter { std::string(name) + "EG", &weight.eg });
}

static std::vector<TuningParameter> selectParameters(HCEWeightTable& tbl, HCEParameterMask mask) {
    std::vector<TuningParameter> ret;

    if (mask & BIT(HCEP_MATERIAL)) {
        for (PieceType pt: { PT_PAWN, PT_KNIGHT, PT_BISHOP, PT_ROOK, PT_QUEEN }) {
            addParameter(ret, std::string("Material") + getPieceTypeName(pt), tbl.material[pt]);
        }
    }

    if (mask & BIT(HCEP_MOBILITY)) {
        int i = 0;
        for (auto& knightMobilityScore: tbl.knightMobilityScore) {
            addParameter(ret, "KnightMobility" + utils::toString(i), knightMobilityScore);
            i++;
        }

        i = 0;
        for (auto& bishopMobilityScore: tbl.bishopMobilityScore) {
            addParameter(ret, "BishopMobility" + utils::toString(i), bishopMobilityScore);
            i++;
        }

        i = 0;
        for (auto& rookMobilityScore: tbl.rookVerticalMobilityScore) {
            addParameter(ret, "RookVerticalMobility" + utils::toString(i), rookMobilityScore);
            i++;
        }

        i = 0;
        for (auto& rookMobilityScore: tbl.rookHorizontalMobilityScore) {
            addParameter(ret, "RookHorizontalMobility" + utils::toString(i), rookMobilityScore);
            i++;
        }
    }

    if (mask & BIT(HCEP_KNIGHT_OUTPOSTS)) {
        addParameter(ret, "KnightOutpost", tbl.knightOutpostScore);
    }

    if (mask & BIT(HCEP_BACKWARD_PAWNS)) {
        addParameter(ret, "BackwardPawn", tbl.backwardPawnScore);
    }

    if (mask & BIT(HCEP_ISOLATED_PAWNS)) {
        addParameter(ret, "IsolatedPawn", tbl.isolatedPawnScore);
    }

    if (mask & BIT(HCEP_PASSED_PAWNS)) {
        int i = 0;
        for (auto& passerScore: tbl.passedPawnScore) {
            addParameter(ret, "PassedPawn" + utils::toString(i), passerScore);
            i++;
        }
    }

    if (mask & BIT(HCEP_KING_PAWN_DISTANCE)) {
        addParameter(ret, "KingPawnDistance", tbl.kingPawnDistanceScore);
    }

    return ret;
}

static void localSearch(Evaluator& eval,
                        const std::vector<TuningSamplePosition>& posData,
                        TuningParameter& param) {
    int& paramValue = *param.ptr;
    int initialValue = paramValue;
    int sign = 0;
    double initialError = averageEvaluationError(eval, posData, paramValue);

    double upper = averageEvaluationError(eval, posData, ++paramValue);
    double lower = averageEvaluationError(eval, posData, paramValue -= 2);

    if (upper < initialError) {
        sign = 1;
    }
    else if (lower < initialError) {
        sign = -1;
    }
    else {
        // Parameter is already well optimized
        return;
    }

    double currentError = initialError;
    double previousError;
    do {
        previousError = currentError;

        currentError = averageEvaluationError(eval, posData, paramValue += sign);
    } while (currentError < previousError);

    paramValue -= sign; // Retract last parameter change

    std::cout << "Local search for parameter " << param.parameterName << " finished." << std::endl;
    std::cout << "Previous value: " << initialValue << " (err " << initialError << ")" << std::endl;
    std::cout << "New value: " << paramValue << " (err " << previousError << ")" << std::endl;
}

HCEWeightTable tune(const HCEWeightTable& tbl,
                    const std::vector<TuningSamplePosition>& positions,
                    const TuningSettings& settings) {
    HCEWeightTable newTbl = tbl;
    HandCraftedEvaluator hce;
    hce.setWeights(newTbl);
    QuiesceEvaluator eval(&hce);

    auto paramMask = settings.parametersMask;

    std::cout << "Tuning process started." << std::endl;

    std::cout << "Selecting parameters..." << std::endl;
    auto params = selectParameters(newTbl, paramMask);

    for (auto& p: params) {
        localSearch(eval, positions, p);
    }

    return newTbl;
}

std::vector<TuningSamplePosition> fetchSamplePositionsFromCSV(std::istream& stream) {
    std::vector<TuningSamplePosition> vec;
    std::string line;

    while (std::getline(stream, line)) {
        std::vector<std::string_view> tokens;
        strutils::split(line, tokens, ",");
        if (tokens.size() != 2) {
            continue;
        }

        std::string_view fen = tokens[0];
        std::string_view resStr = tokens[1];

        ChessResult res = resStr == "w" ? RES_WIN_CHECKMATE
                                 : resStr == "b" ? RES_LOSS_CHECKMATE
                                              : RES_DRAW_REPETITION;

        vec.emplace_back(Position::fromFen(fen).value(), res);
    }

    return vec;
}

}