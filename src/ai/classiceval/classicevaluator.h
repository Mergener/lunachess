#ifndef LUNA_AI_CLASSIC_EVALUATOR_H
#define LUNA_AI_CLASSIC_EVALUATOR_H

#include <iostream>
#include <array>
#include <functional>

#include <nlohmann/json.hpp>

#include "../evaluator.h"
#include "hotmap.h"

#include "../../endgame.h"

namespace lunachess::ai {

struct ScoreTable {
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ScoreTable, materialScore, hotmapGroups,
                                       kingHotmap, xrayScores, mobilityScores,
                                       bishopPairScore, outpostScore, goodComplexScore,
                                       nearKingAttacksScore, pawnShieldScore,
                                       kingOnOpenFileScore, kingNearOpenFileScore,
                                       kingOnSemiOpenFileScore, kingNearSemiOpenFileScore,
                                       supportPawnsHotmap, passersHotmap, connectedPassersHotmap);

    /**
     * Score given for the mere existence of pieces of our color.
     * Score is specific for each piece type.
     */
    std::array<int, PT_COUNT> materialScore {};

    // Activity scores:
    std::array<KingSquareHotmapGroup, PT_COUNT - 2> hotmapGroups;
    inline KingSquareHotmapGroup& getHotmap(PieceType pt) { return hotmapGroups[pt - 1]; }
    inline const KingSquareHotmapGroup& getHotmap(PieceType pt) const { return hotmapGroups[pt - 1]; }

    Hotmap kingHotmap;

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
    std::array<int, PT_COUNT> mobilityScores{};

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

    // King safety scores:
    std::array<int, PT_COUNT> nearKingAttacksScore{};
    int pawnShieldScore = 0;
    int kingOnOpenFileScore = 0;
    int kingNearOpenFileScore = 0;
    int kingOnSemiOpenFileScore = 0;
    int kingNearSemiOpenFileScore = 0;

    /**
     * Hotmap for support pawn scores.
     * A pawn is a support pawn if it has pawns of the
     * same color in adjacent files.
     */
    Hotmap supportPawnsHotmap;

    /**
     * Hotmap for passed pawn scores.
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
    int getDrawScore() const override;
    int evaluate(const Position& pos) const override;
    int evaluateShallow(const Position& pos) const override;

    int getGamePhaseFactor(const Position& pos) const;

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
    static Bitboard getPassedPawns(const Position& pos, Color c);
    static Bitboard getChainPawns(const Position& pos, Color c);
    int evaluateBlockingPawns(const Position& pos, Color c, int gpf) const;
    int evaluateChainsAndPassers(const Position& pos, Color c, int gpf) const;

    // Activity
    int evaluatePawnComplex(const Position& pos, Color c, int gpf) const;
    int evaluatePlacement(const Position& pos, Color c, int gpf) const;
    int evaluateOutposts(const Position& pos, Color c, int gpf) const;
    int evaluateMobility(const Position& pos, Color c, int gpf) const;
    int evaluateBishopPair(const Position& pos, Color c, int gpf) const;
    int evaluateXrays(const Position& pos, Color c, int gpf) const;

    int evaluateClassic(const Position& pos) const;
    int evaluateEndgame(const Position& pos, EndgameData egData) const;
    int evaluateKPK(const Position& pos, Color lhs) const;
    int evaluateKBNK(const Position& pos, Color lhs) const;

    static void generateNewMgTable();
    static void generateNewEgTable();
};

std::istream& operator>>(std::istream& stream, ScoreTable& scores);
std::ostream& operator<<(std::ostream& stream, const ScoreTable& scores);

}

#endif // LUNA_AI_CLASSIC_EVALUATOR_H