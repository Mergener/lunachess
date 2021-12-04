#include "openingbook.h"

#include <stack>

namespace lunachess::ai {

class OpeningBookBuilder {
public:
	void push(Square src, Square dst, PieceType promotionTarget = PieceType::None) {
		Move move = Move(m_Pos, src, dst, promotionTarget);
		m_Pos.makeMove(move);
		m_Moves.push(move);
	}

	void pushAndRegister(Square src, Square dst, PieceType promotionTarget = PieceType::None) {
		registerMove(src, dst, promotionTarget);
		push(src, dst, promotionTarget);
	}

	void registerMove(Square src, Square dest, PieceType promotionTarget = PieceType::None) {
		m_Ret.setMoveForPosition(m_Pos, Move(m_Pos, src, dest, promotionTarget));
	}

	void pop() {
		Move move = m_Moves.top();
		m_Pos.undoMove(move);
		m_Moves.pop();
	}

	OpeningBook get() const {
		return m_Ret;
	}

	Position& position() {
		return m_Pos;
	}

	OpeningBookBuilder() {
		m_Pos = Position::getInitialPosition();
	}

private:
	Position m_Pos;
	OpeningBook m_Ret;
	std::stack<Move> m_Moves;
};

/*
	Default opening book:

d4
    d5
e4
	e5
c4
    e5

*/

OpeningBook OpeningBook::getDefault() {
	OpeningBookBuilder ret;

	// 1. d4 d5
	ret.push(SQ_D2, SQ_D4);
	ret.registerMove(SQ_D7, SQ_D5);
	ret.pop();

	// 1. e4 e5 2. Ke2
	ret.pushAndRegister(SQ_E2, SQ_E4);
	ret.pushAndRegister(SQ_C7, SQ_C5);
	ret.pop();
	ret.pop();

	// 1. c4 e5
	ret.push(SQ_C2, SQ_C4);
	ret.registerMove(SQ_E7, SQ_E5);
	ret.pop();


	return ret.get();
}

}