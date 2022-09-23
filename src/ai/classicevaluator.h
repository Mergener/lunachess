#ifndef LUNA_AI_CLASSIC_EVALUATOR_H
#define LUNA_AI_CLASSIC_EVALUATOR_H

#include <iostream>
#include <array>
#include <functional>

#include "evaluator.h"
#include "hotmap.h"

#include "../endgame.h"

namespace lunachess::ai {

struct ScoreTable {
    std::array<int, PT_COUNT> materialScore;

    // Activity scores:
    std::array<KingSquareHotmapGroup, PT_COUNT - 2> hotmapGroups;
    inline KingSquareHotmapGroup& getHotmap(PieceType pt) { return hotmapGroups[pt - 1]; }
    inline const KingSquareHotmapGroup& getHotmap(PieceType pt) const { return hotmapGroups[pt - 1]; }

    Hotmap kingHotmap;

    std::array<int, PT_COUNT> xrayScores;
    int mobilityScore = 0;
    int bishopPairScore = 0;
    int outpostScore = 0;
    int goodComplexScore = 0;

    // King safety scores:
    std::array<int, PT_COUNT> tropismScore;
    int pawnShieldScore = 0;
    int kingOnOpenFileScore = 0;
    int kingNearOpenFileScore = 0;
    int kingOnSemiOpenFileScore = 0;
    int kingNearSemiOpenFileScore = 0;
    int nearKingSquareAttacksScore = 0;

    // Pawn structure scores:
    int doublePawnScore = 0;
    int pawnChainScore = 0;
    int passerPercentBonus = 0;
    int outsidePasserPercentBonus = 0;

    inline ScoreTable() noexcept {
        std::memset(reinterpret_cast<void*>(this), 0, sizeof(*this));
    }

    inline void foreachHotmapGroup(std::function<void(KingSquareHotmapGroup&, PieceType)> fn) {
        for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
            fn(hotmapGroups[pt - 1], pt);
        }
    }

    inline void foreachHotmapGroup(std::function<void(const KingSquareHotmapGroup&, PieceType)> fn) const {
        for (PieceType pt = PT_PAWN; pt < PT_KING; ++pt) {
            fn(hotmapGroups[pt - 1], pt);
        }
    }
};

class ClassicEvaluator : public Evaluator {
    static ScoreTable defaultMgTable;
    static ScoreTable defaultEgTable;

public:
    virtual int getDrawScore() const override;
    virtual int evaluate(const Position& pos) const override;
    virtual int evaluateShallow(const Position& pos) const override;

    int getGamePhaseFactor(const Position& pos) const;

    struct PasserData {
        Bitboard allPassers = 0;
        Bitboard outsidePassers = 0;

        int passerPercentBonus;
        int outsidePasserPercentBonus;

        inline bool isPasser(Square s) const { return allPassers.contains(s); }
        inline bool isOutsidePasser(Square s) const { return outsidePassers.contains(s); }

        inline int multiplyScore(int score, bool isPasser, bool isOutsidePasser) const {
            int mult = 100;

            if (isPasser) {
                mult += passerPercentBonus;
                if (isOutsidePasser) {
                    mult += outsidePasserPercentBonus;
                }
            }

            int ret = (score * mult) / 100;

            return ret;
        }

        inline int multiplyScoreIfPasser(Square square, int score) const {
            return multiplyScore(score, isPasser(square), isOutsidePasser(square));
        }
    };
    inline PasserData getPasserData(const Position& pos, Color c) const {
        return getPasserData(pos, c, getGamePhaseFactor(pos));
    }

    inline ScoreTable& getMiddlegameScores() { return m_MgScores; }
    inline const ScoreTable& getMiddlegameScores() const { return m_MgScores; }
    inline void setMiddlegameScores(const ScoreTable& scores) {
        m_MgScores = scores;
    }

    inline ScoreTable& getEndgameScores() { return m_EgScores; }
    inline const ScoreTable& getEndgameScores() const { return m_EgScores; }
    inline void setEndgameScores(const ScoreTable& scores) {
        m_EgScores = scores;
    }

    ClassicEvaluator();

    static void initialize();

private:
    ScoreTable m_MgScores;
    ScoreTable m_EgScores;

    int evaluateMaterial(const Position& pos, Color c, int gpf) const;

    // King safety
    int evaluateTropism(const Position& pos, Color c, int gpf) const;
    int evaluatePawnShield(const Position& pos, Color c, int gpf) const;
    int evaluateKingExposure(const Position& pos, Color c, int gpf) const;
    int evaluateNearKingAttacks(const Position& pos, Color c, int gpf) const;

    // Pawn structure

    PasserData getPasserData(const Position& pos, Color c, int gpf) const;

    int evaluateBlockingPawns(const Position& pos, Color c, int gpf) const;
    int evaluatePawnChains(const Position& pos, Color c, int gpf, const PasserData& pd) const;

    // Activity
    int evaluatePawnComplex(const Position& pos, Color c, int gpf) const;
    int evaluatePlacement(const Position& pos, Color c, int gpf, const PasserData& pd) const;
    int evaluateOutposts(const Position& pos, Color c, int gpf) const;
    int evaluateMobility(const Position& pos, Color c, int gpf) const;
    int evaluateBishopPair(const Position& pos, Color c, int gpf) const;
    int evaluateXrays(const Position& pos, Color c, int gpf) const;

    int evaluateClassic(const Position& pos) const;
    int evaluateEndgame(const Position& pos, EndgameData egData) const;
    int evaluateKPK(const Position& pos, Color lhs) const;

    static void generateNewMgTable();
    static void generateNewEgTable();
};

std::istream& operator>>(std::istream& stream, ScoreTable& scores);
std::ostream& operator<<(std::ostream& stream, const ScoreTable& scores);

}

#endif // LUNA_AI_CLASSIC_EVALUATOR_H