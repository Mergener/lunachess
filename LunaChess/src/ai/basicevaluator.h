#ifndef BASIC_EVALUATOR_H
#define BASIC_EVALUATOR_H

#include "scorestable.h"
#include "evaluator.h"

namespace lunachess::ai {

class BasicEvaluator : public Evaluator {
public:
	int evaluate(const Position& pos) const override;
	int getDrawScore() const override;

	bool isKnightOutpost(const Position& pos, Square sq, Side side) const;

	BasicEvaluator();

private:
	ScoresTable m_OpScoresTable;
	ScoresTable m_EndScoresTable;

	int interpolateScores(int earlyScore, int lateScore, int gamePhaseFactor) const;

	int getPawnChainScore(const Position& pos, Side side, int gpp) const;
	int getBishopPairScore(const Position& pos, Side side, int gpp) const;
	int getGamePhaseFactor(const Position& pos) const;
	int getKingSafetyScore(const Position& pos, Side side, int gpp) const;
	int getBlockingPawnScore(const Position& pos, Side side, int gpp) const;
	int getPawnColorScore(const Position& pos, Side side, int gpp) const;
	int getKnightOutpostScore(const Position& pos, Side side, int gpp) const;
};

}

#endif // BASIC_EVALUATOR_H