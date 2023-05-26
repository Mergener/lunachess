#ifndef LUNA_AI_CLASSIC_EVALUATOR_H
#define LUNA_AI_CLASSIC_EVALUATOR_H

#include <iostream>
#include <array>
#include <functional>

#include <nlohmann/json.hpp>

#include "../evaluator.h"
#include "evalscores.h"
#include "../../bits.h"
#include "../../utils.h"

#include "../../endgame.h"

namespace lunachess::ai {

class PieceSquareTable {
public:
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PieceSquareTable, m_Values);

    inline short& valueAt(Square s, Color pov) {
        return m_Values[squareToIdx(s, pov)];
    }

    inline short valueAt(Square s, Color pov) const {
        return m_Values[squareToIdx(s, pov)];
    }

    inline PieceSquareTable() noexcept {
        std::fill(m_Values.begin(), m_Values.end(), 0);
    }

    inline PieceSquareTable(const std::array<short, SQ_COUNT>& values)
            : m_Values(values) {
    }

    inline PieceSquareTable(const std::initializer_list<short>& values) {
        size_t i;
        auto it = values.begin();
        for (i = 0; i < values.size() && i < SQ_COUNT; ++i) {
            m_Values[i] = *it;
            ++it;
        }

        std::fill(m_Values.begin() + i, m_Values.end(), 0);
    }

    using Iterator = short*;
    using ConstIterator = const short*;

    inline Iterator begin() { return &m_Values[0]; }
    inline Iterator end() { return &m_Values[SQ_COUNT]; }
    inline ConstIterator cbegin() const { return &m_Values[0]; }
    inline ConstIterator cend() const { return &m_Values[SQ_COUNT]; }

private:
    std::array<short, SQ_COUNT> m_Values;

    inline static Square squareToIdx(Square s, Color c) {
        return c == CL_WHITE ? mirrorVertically(s) : s;
    }
};

inline std::ostream& operator<<(std::ostream& stream, const PieceSquareTable& hotmap) {
    stream << "      ";
    for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
        stream << getFileIdentifier(f) << "     ";
    }
    for (BoardRank r = RANK_8; r >= RANK_1; --r) {
        stream << std::endl;
        stream << getRankIdentifier(r) << " |";
        for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
            int valueAt = hotmap.valueAt(getSquare(f, r), CL_WHITE);
            std::cout << std::setw(6)
                      << std::fixed
                      << std::setprecision(2)
                      << double(valueAt) / 1000;
        }
    }

    return stream;
}

enum HCEParameter {
    HCEP_MATERIAL,
    HCEP_MOBILITY,
    HCEP_PASSED_PAWNS,
    HCEP_BACKWARD_PAWNS,

    HCEP_PARAM_COUNT,
};

using HCEParameterMask = ui64;

static constexpr HCEParameterMask HCEPM_ALL = ~0;

template<HCEParameterMask MASK = HCEPM_ALL>
class HandCraftedEvaluator : public Evaluator {
private:
    inline static constexpr int PIECE_VALUE_TABLE[] = {
        0, 1, 3, 3, 5, 10, 0
    };

    inline static constexpr int OPENING_GPF =
        PIECE_VALUE_TABLE[PT_PAWN] * 8 * 2 +
        PIECE_VALUE_TABLE[PT_KNIGHT] * 2 * 2 +
        PIECE_VALUE_TABLE[PT_BISHOP] * 2 * 2 +
        PIECE_VALUE_TABLE[PT_ROOK] * 2 * 2 +
        PIECE_VALUE_TABLE[PT_QUEEN] * 1 * 2;

public:
    inline int evaluate() const override {
        return 0;
    }

    inline int getDrawScore() const override { return 0; }

private:
    static int interpolateScores(int mg, int eg, int gpf) {
        return (mg * gpf) / OPENING_GPF + (eg * (OPENING_GPF - gpf)) / OPENING_GPF;
    }
};

}

#endif // LUNA_AI_CLASSIC_EVALUATOR_H