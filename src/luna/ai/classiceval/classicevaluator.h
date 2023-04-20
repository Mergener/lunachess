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
    template<int N = 0>
    struct Weight {
        static constexpr int ARR_SIZE = std::max(1, N);

        std::array<int, ARR_SIZE> mg;
        std::array<int, ARR_SIZE> eg;

        Weight() = default;
        Weight(const std::array<int, ARR_SIZE>& mg,
               const std::array<int, ARR_SIZE>& eg)
           : mg(mg), eg(eg) {}
    };

    struct WeightsTable {
        Weight<> material[PT_COUNT];
        Weight<> centralPawns[PT_COUNT];
    };

    inline int evaluate() const override {
        return m_Eval * m_Sign;
    }

    inline int getDrawScore() const override { return 0; }

private:
    WeightsTable m_Weights;

    // White POV eval scores
    int m_Eval = 0;
    int m_MgEval = 0;
    int m_EgEval = 0;

    int m_Sign = 1;
    int m_GPF = 80;

    static int interpolateScores(int mg, int eg, int gpf) {
        return (mg * gpf) / OPENING_GPF + (eg * (OPENING_GPF - gpf)) / OPENING_GPF;
    }

    template<int N = 0>
    void applyWeight(const Weight<N>& w, int n) {
        if constexpr (N == 0) {
            m_MgEval += w.mg[0] * n * m_Sign;
            m_EgEval += w.eg[0] * n * m_Sign;
        }
        else {
            int sign = m_Sign * utils::sign(n);
            n = std::abs(std::max(n, N));
            m_MgEval += w.mg[n] * sign;
            m_EgEval += w.eg[n] * sign;
        }
    }

    void handleNoisyMoves(Move move) {
        if (move.is<MTM_CAPTURE>()) {
            auto capturedPieceType = move.getCapturedPiece().getType();
            if constexpr (MASK & BIT(HCEP_MATERIAL)) {
                applyWeight(m_Weights.material[capturedPieceType], 1);
            }
        }

        if (move.is<MTM_PROMOTION>()) {
            auto promPieceType = move.getPromotionPiece();
            if constexpr (MASK & BIT(HCEP_MATERIAL)) {
                applyWeight(m_Weights.material[PT_PAWN], -1); // Lost a pawn to promote...
                applyWeight(m_Weights.material[promPieceType], 1); // ...but gained a new piece
            }
        }
    }

    void onPieceEnterSquare(Piece piece, Square s) {
        int sign = piece.getColor() == CL_WHITE ? 1 : -1;

        // Apply material score
        if constexpr (MASK & BIT(HCEP_MATERIAL)) {
            applyWeight(m_Weights.material[piece.getType()], sign);
        }
    }

    void onPieceLeaveSquare(Piece piece, Square s) {
        int sign = piece.getColor() == CL_WHITE ? 1 : -1;

        // Apply material score
        if constexpr (MASK & BIT(HCEP_MATERIAL)) {
            applyWeight(m_Weights.material[piece.getType()], -sign);
        }
    }

    void evaluationStep(Move move) {
        handleNoisyMoves(move);
        onPieceEnterSquare(move.getSourcePiece(), move.getDest())
    }

protected:
    void onSetPosition(const Position &pos) override {
        m_Sign = pos.getColorToMove() == CL_WHITE ? 1 : -1;
        m_Eval = 0;
        m_MgEval = 0;
        m_EgEval = 0;
        m_GPF = 0;

        for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
            auto nWhite = pos.getBitboard(Piece(CL_WHITE, pt)).count();
            auto nBlack = pos.getBitboard(Piece(CL_BLACK, pt)).count();
            m_GPF += PIECE_VALUE_TABLE[pt] * (nBlack + nWhite);
        }

        for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
            auto nWhite = pos.getBitboard(Piece(CL_WHITE, pt)).count();
            auto nBlack = pos.getBitboard(Piece(CL_BLACK, pt)).count();

            if constexpr (MASK & BIT(HCEP_MATERIAL)) {
                applyWeight(m_Weights.material[pt], nWhite);
                applyWeight(m_Weights.material[pt], -nBlack);
            }
        }

        m_GPF = std::clamp(m_GPF, 0, OPENING_GPF);

        m_Eval = interpolateScores(m_MgEval, m_EgEval, m_GPF);
    }

    void onMakeMove(Move move) override {
        if (move.is<MTM_CAPTURE>()) {
            auto capturedPieceType = move.getCapturedPiece().getType();
            m_GPF -= PIECE_VALUE_TABLE[capturedPieceType];
        }

        if (move.is<MTM_PROMOTION>()) {
            auto promPieceType = move.getPromotionPiece();
            m_GPF -= PIECE_VALUE_TABLE[PT_PAWN];
            m_GPF += PIECE_VALUE_TABLE[promPieceType];
        }

        evaluationStep(move);

        m_Sign *= -1;

        m_Eval = interpolateScores(m_MgEval, m_EgEval, m_GPF);
    }

    void onUndoMove(Move move) override {
        evaluationStep(move);

        if (move.is<MTM_CAPTURE>()) {
            auto capturedPieceType = move.getCapturedPiece().getType();
            m_GPF += PIECE_VALUE_TABLE[capturedPieceType];
        }

        if (move.is<MTM_PROMOTION>()) {
            auto promPieceType = move.getPromotionPiece();
            m_GPF += PIECE_VALUE_TABLE[PT_PAWN];
            m_GPF -= PIECE_VALUE_TABLE[promPieceType];
        }

        m_Sign *= -1;

        m_Eval = interpolateScores(m_MgEval, m_EgEval, m_GPF);
    }

    void onMakeNullMove() override {
        m_Sign *= -1;
    }

    void onUndoNullMove() override {
        m_Sign *= -1;
    }

public:
    HandCraftedEvaluator() {
        m_Weights.material[PT_PAWN] = Weight({1000}, {1100});
        m_Weights.material[PT_KNIGHT] = Weight({3000}, {3300});
        m_Weights.material[PT_BISHOP] = Weight({3300}, {3660});
        m_Weights.material[PT_ROOK] = Weight({5000}, {5500});
        m_Weights.material[PT_QUEEN] = Weight({9000}, {9900});

        m_Weights.centralPawns = Weight({180, 270, 350}, {0});
    }
};

}

#endif // LUNA_AI_CLASSIC_EVALUATOR_H