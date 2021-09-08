#ifndef MOVELIST_H
#define MOVELIST_H

#include "move.h"

namespace lunachess {

template <typename T, int SIZE>
class StaticList {
public:
	using Iterator = T*;
	using ConstIterator = const T*;

	inline constexpr int capacity() const { return SIZE; }

	inline Iterator begin() { return &m_List[0]; }
	Iterator end() { return &m_List[m_Count]; }

	inline ConstIterator cbegin() const { return &m_List[0]; }
	ConstIterator cend() const { return &m_List[m_Count]; }

	inline bool contains(T val) const {
		for (auto it = cbegin(); it != cend(); ++it) {
			if ((*it) == val) {
				return true;
			}
		}
		return false;
	}

	inline void add(const T& val) {
		LUNA_ASSERT(m_Count < capacity(), 
			"Tried adding beyond capacity. (capacity was " << capacity() << ")");

		m_List[m_Count] = val;
		m_Count++;
	}

	inline void clear() {
		m_Count = 0;
	}

	inline T& operator[](int index) {
		return m_List[index];
	}

	inline void removeAt(int index) {
		LUNA_ASSERT(index >= 0, "Index must be greater than or equal to 0. (got " << index << ")");
		LUNA_ASSERT(index < m_Count, "Index must be less than count (got " << index << ", count was " << m_Count << ")");

		m_Count--;
		for (int i = index; i < m_Count; ++i) {
			m_List[i] = m_List[i + 1];
		}
	}

	inline bool empty() const {
		return m_Count == 0;
	}

	inline int count() const {
		return m_Count;
	}

	inline int indexOf(const T& val) {
		for (int i = 0; i < m_Count; ++i) {
			if (m_List[i] == val) {
				return i;
			}
		}
		return -1;
	}

	inline void removeAtUnordered(int index) {
		LUNA_ASSERT(index >= 0, "Index must be greater than or equal to 0. (got " << index << ")");
		LUNA_ASSERT(index < m_Count, "Index must be less than count (got " << index << ", count was " << m_Count << ")");

		m_List[index] = m_List[m_Count - 1];
		m_Count--;
	}

	inline const T& operator[](int index) const {
		LUNA_ASSERT(index >= 0, "Index must be greater than or equal to 0. (got " << index << ")");
		LUNA_ASSERT(index < m_Count, "Index must be less than count (got " << index << ", count was " << m_Count << ")");

		return m_List[index];
	}

	inline StaticList()
		: m_Count(0) {
	}

	inline StaticList(const StaticList& other) {
		std::memcpy(m_List, other.m_List, m_Count * sizeof(T));
		m_Count = other.m_Count;
	}

	inline StaticList& operator=(const StaticList& other) {
		std::memcpy(m_List, other.m_List, m_Count * sizeof(T));
		m_Count = other.m_Count;
		return *this;
	}

private:
	T m_List[SIZE];
	int m_Count = 0;
};

using MoveList = StaticList<Move, 256>;

}

#endif // MOVELIST_H