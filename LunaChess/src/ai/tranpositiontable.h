#ifndef LUNA_TRANSPOSITION_TABLE_H
#define LUNA_TRANSPOSITION_TABLE_H

#include <unordered_map>

#include "../core/position.h"
#include "../core/types.h"
#include "../core/zobrist.h"

namespace lunachess::ai {

class TranspositionTable {
public:
	enum EntryType {
		EXACT,
		LOWERBOUND,
		UPPERBOUND
	};

	struct Entry {
		ui64 zobristKey;
		EntryType type;
		int score;
		int depth;
		Move move;
	};

public:
	using Container = std::unordered_map<ui64, Entry>;
	using Iterator = Container::iterator;
	using ConstIterator = Container::const_iterator;

	Iterator begin() { return m_Entries.begin(); }
	Iterator end() { return m_Entries.end(); }
	ConstIterator cbegin() const { return m_Entries.cbegin(); }
	ConstIterator cend() const { return m_Entries.cend(); }

	/**
		Adds a given entry to the transposition table, except if
		an entry for the same position with higher depth exists.
	*/
	void maybeAdd(const Entry& entry);

	inline bool tryGet(const Position& pos, Entry& entry) const {
		return tryGet(pos.getZobristKey(), entry);
	}

	inline void reserve(int size) {
		m_Entries.reserve(size);
	}

	bool tryGet(ui64 posKey, Entry& entry) const;

	inline void remove(const Position& pos) {
		remove(pos.getZobristKey());
	}

	void remove(ui64 posKey);

	inline void clear() {
		m_Entries.clear();
	}	

	TranspositionTable();

private:
	Container m_Entries;
};

}

#endif // LUNA_TRANSPOSITION_TABLE_H