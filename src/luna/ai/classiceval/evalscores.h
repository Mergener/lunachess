#ifndef LUNA_EVALSCORES_H
#define LUNA_EVALSCORES_H

#include <iostream>
#include <cstring>
#include <array>
#include <initializer_list>
#include <iomanip>

#include <nlohmann/json.hpp>

#include "../../types.h"

namespace lunachess::ai {

class KingSquareHotmapGroup;


class Hotmap {
public:
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Hotmap, m_Values);

    inline short& valueAt(Square s, Color pov) {
        return m_Values[squareToIdx(s, pov)];
    }

    inline short valueAt(Square s, Color pov) const {
        return m_Values[squareToIdx(s, pov)];
    }

    inline Hotmap() noexcept {
        std::fill(m_Values.begin(), m_Values.end(), 0);
    }

    inline Hotmap(const std::array<short, SQ_COUNT>& values)
            : m_Values(values) {
    }

    inline Hotmap(const std::initializer_list<short>& values) {
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

//std::ostream& operator<<(std::ostream& stream, const Hotmap& hotmap);

struct ScoreTable {
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ScoreTable, materialScore,
                                       kingHotmap, xrayScores, mobilityScores,
                                       bishopPairScore, outpostScore, goodComplexScore,
                                       nearKingAttacksScore,
                                       chainedPawnsHotmap, passersHotmap, connectedPassersHotmap);

    /**
     * Score given for the mere existence of pieces of our color.
     * Score is specific for each piece type.
     */
    std::array<int, PT_COUNT> materialScore {};

    /**
     * Score given for every opposing piece that is either
     * under the line of sight or xray of one of our sliding pieces.
     * The score is specific to each piece type of piece being xrayed.
     * Ex: a score of 50 for PT_ROOK will give us 50 mps for every xray
     * being applied on an opposing rook.
     */
    std::array<int, PT_COUNT> xrayScores{};

    /**
     * Score given for each square being attacked by a piece of a color.
     * The score is specific to each piece type. Does not apply to pawns or
     * kings.
     */
    static constexpr int N_MOBILITY_SCORES = 12;
    std::array<std::array<int, N_MOBILITY_SCORES>, PT_COUNT> mobilityScores{};
    std::array<int, PT_COUNT> nearKingAttacksScore{};
    std::array<int, PT_COUNT> kingExposureScores{};

    /**
     * Score given for having two bishops of opposite colors.
     */
    int bishopPairScore = 0;

    /**
     * Score given for each knight that is placed on a square that has no
     * opposing pawns on adjacent files that can be pushed to kick
     * them away.
     */
    int outpostScore = 0;

    /**
     * Score given for each pawn that is placed on a score that favors
     * our bishop, if we only have one.
     */
    int goodComplexScore = 0;


    Hotmap kingHotmap;
    Hotmap pawnsHotmap;

    /**
     * PieceSquareTable for support pawn scores.
     * A pawn is a support pawn if it has pawns of the
     * same color in adjacent files.
     */
    Hotmap chainedPawnsHotmap;

    /**
     * PieceSquareTable for passed pawn scores.
     * A pawn is passed (aka a passer) when no opposing pawn can either
     * block it or capture it further down its file.
     */
    Hotmap passersHotmap;

    /**
     * Extra score given to pawns that are both passers and support pawns.
     */
    Hotmap connectedPassersHotmap;

    /**
     * Score given to pawns that are blocking other pawns of the same color in the file.
     */
    int blockingPawnScore = 0;

    void toParameterVector(std::vector<int>& v) const;
    void fromParameterVector(const std::vector<int>& v);

    inline ScoreTable() noexcept {
        std::memset(reinterpret_cast<void*>(this), 0, sizeof(*this));
    }

    static const ScoreTable& getDefaultMiddlegameTable();
    static const ScoreTable& getDefaultEndgameTable();
};

void initializeEvalScores();

}

#endif // LUNA_EVALSCORES_H