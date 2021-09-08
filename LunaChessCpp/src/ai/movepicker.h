#ifndef LUNA_MOVEPICKER_H
#define LUNA_MOVEPICKER_H

#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>

#include "tranpositiontable.h"
#include "evaluator.h"
#include "basicevaluator.h"
#include "openingbook.h"

#include "../core/move.h"
#include "../core/position.h"

namespace lunachess::ai {

class MovePicker {
private:
	inline void generateQGDBook() {
		Position initialPos = Position::getInitialPosition();
		Position pos = initialPos;

		Move move = Move(pos, SQ_D2, SQ_D4);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_D7, SQ_D5);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_C2, SQ_C4);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_E7, SQ_E6);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_B1, SQ_C3);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_G8, SQ_F6);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_C1, SQ_G5);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_F8, SQ_E7);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

	}

	inline void generateKingsIndianBook() {
		Position initialPos = Position::getInitialPosition();
		Position pos = initialPos;

		Move move = Move(pos, SQ_D2, SQ_D4);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_G8, SQ_F6);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_C2, SQ_C4);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_D7, SQ_D6);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_B1, SQ_C3);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_G7, SQ_G6);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_E2, SQ_E4);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_F8, SQ_G7);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

	}

	inline void generateRuyLopezBook() {
		Position initialPos = Position::getInitialPosition();
		Position pos = initialPos;

		Move move = Move(pos, SQ_E2, SQ_E4);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_E7, SQ_E5);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_G1, SQ_F3);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_B8, SQ_C6);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_F1, SQ_B5);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_A7, SQ_A6);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_B5, SQ_A4);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_B7, SQ_B5);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

	}

	inline void generateSicilianBook() {
		Position initialPos = Position::getInitialPosition();
		Position pos = initialPos;

		Move move = Move(pos, SQ_E2, SQ_E4);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_C7, SQ_C5);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);

		move = Move(pos, SQ_G1, SQ_F3);
		m_OpBook.setMoveForPosition(pos, move);
		pos.makeMove(move);
	}

public:
	using DrawList = Position::DrawList;
	using EvalT = BasicEvaluator;

	static constexpr int HIGH_BETA = 999999;
	static constexpr int MATE_IN_ONE_SCORE = 300000;
	static constexpr int FORCED_MATE_THRESHOLD = 250000;

	std::tuple<Move, int> pickMove(const Position& pos, int depth);

	EvalT& getEvaluator() { return m_Eval; }
	const EvalT& getEvaluator() const { return m_Eval; }

	inline MovePicker() {
		//generateSicilianBook();
		//generateSicilianBook();
		//generateRuyLopezBook();
		//generateKingsIndianBook();
		//generateQGDBook();
	}

private:
	static constexpr int N_KILLER_MOVES = 2;
	static constexpr int MAX_DEPTH = 256;

	EvalT m_Eval;
	TranspositionTable m_TT;
	OpeningBook m_OpBook;
	Move m_Killers[N_KILLER_MOVES][MAX_DEPTH];

	std::tuple<Move, int> pickMoveAndScore(Position& pos, int depth, int ply, int alpha, int beta, bool us);
	int quiesce(Position& pos, int ply, int alpha, int beta);
	void orderMoves(MoveList& ml, Move ttMove, int ply);
	void orderMovesQuiesce(MoveList& ml, int ply);
	void storeKillerMove(Move move, int ply);
};



}

#endif // LUNA_MOVEPICKER_H