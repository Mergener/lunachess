#ifndef LUNA_MOVEPICKER_H
#define LUNA_MOVEPICKER_H

#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>
#include <atomic>

#include "tranpositiontable.h"
#include "evaluator.h"
#include "basicevaluator.h"
#include "openingbook.h"

#include "../core/move.h"
#include "../core/position.h"

namespace lunachess::ai {

/**
 * An alpha-beta pruning based move searcher.
 */
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
	using EvalT = BasicEvaluator;

	static constexpr int HIGH_BETA = 999999;
	static constexpr int MATE_IN_ONE_SCORE = 300000;
	static constexpr int FORCED_MATE_THRESHOLD = 250000;

    /**
     * Evaluates a given position, while also suggesting a move.
     *
     * @remarks
     *  For a single move picker object, this method will only be executed from
     *  a single thread. This means that this methods has a lock that is kept closed
     *  until the end of execution, and any other threads that try to invoke this method
     *  from the same object will have to wait until the lock is set free.
     *
     * @param pos The position to pickMove the move.
     * @param depth The depth of pickMove. Higher depths increase accuracy at the cost of execution time.
     * @return A tuple that contains Luna's move choice and evaluation.
     */
	std::tuple<Move, int> pickMove(const Position& pos, int depth);
    /**
     * @name getEvaluator
     * @brief  This move picker's evaluator.
     * @remarks The evaluator statically evaluates positions.
     * This means that it rates a position without searching for any of the
     * sides moves, only caring about the position of pieces and the total material count.
     */
	inline EvalT& getEvaluator() { return m_Eval; }

    /**
     * @copydoc MovePicker::getEvaluator()
     */
	inline const EvalT& getEvaluator() const { return m_Eval; }

    inline const TranspositionTable& getTranspositionTable() const { return m_TT; }

	inline MovePicker() {
        generateQGDBook();
	}

private:
	static constexpr int N_KILLER_MOVES = 2;
	static constexpr int MAX_DEPTH = 256;
    static constexpr int NULL_MOVE_SEARCH_DEPTH_REDUC = 2;

	EvalT m_Eval;
	TranspositionTable m_TT;
	OpeningBook m_OpBook;
	Move m_Killers[N_KILLER_MOVES][MAX_DEPTH];
    int m_Butterflies[64][64];
    std::atomic_bool m_Lock = false;
    bool m_LastMoveWasNull = false;

    Position m_Position;

	int searchInternal(int depth, int ply, int alpha, int beta, bool us, bool nullMove);
	int quiesce(int ply, int alpha, int beta);
	void orderMoves(MoveList& ml, Move ttMove, int ply);
	void orderMovesQuiesce(MoveList& ml, int ply);
	void storeKillerMove(Move move, int ply);
    bool compareMoves(const Move& a, const Move& b) const;
    bool compareCaptures(Move a, Move b) const;
    bool canSearchNullMove(bool sideInCheck) const;
    bool squareCanBeCapturedByPawn(Square s, Side side) const;

    inline bool isKillerMove(Move m, int depth) const {
        for (int i = 0; i < N_KILLER_MOVES; ++i) {
            if (m == m_Killers[i][depth]) {
                return true;
            }
        }
        return false;
    }

    void reset();
};



}

#endif // LUNA_MOVEPICKER_H