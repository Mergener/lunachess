#ifndef LUNA_OPENING_BOOK_H
#define LUNA_OPENING_BOOK_H

#include <unordered_map>

#include "../core/debug.h"
#include "../core/position.h"

namespace lunachess::ai {

class OpeningBook {
public:
	/**
		Returns a move to be played in the specified position or MOVE_INVALID
		if none is found.
	*/
	inline Move getMoveForPosition(const Position& pos) const {
		auto it = m_Moves.find(pos.getZobristKey());
		if (it == m_Moves.cend()) {
			return MOVE_INVALID;
		}

		auto ret = it->second;
		LUNA_ASSERT(pos.moveIsLegal(ret), "Expects the move to be legal.");
		return ret;
	}

	inline void clear() {
		m_Moves.clear();
	}

	/**
		Sets a move to be played in a specific position.
	*/
	inline void setMoveForPosition(const Position& pos, Move move) {
		LUNA_ASSERT(pos.moveIsLegal(move), "Expects the move to be legal.");
		m_Moves[pos.getZobristKey()] = move;
	}

	static OpeningBook getDefault();

private:
	std::unordered_map<ui64, Move> m_Moves;
};

}

#endif // LUNA_OPENING_BOOK_H