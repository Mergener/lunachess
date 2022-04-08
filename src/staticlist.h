#ifndef LUNA_STATICLIST_H
#define LUNA_STATICLIST_H

#include <type_traits>
#include <iostream>

#include "debug.h"

namespace lunachess {

/**
 * Contiguous, non-resizable memory container for storing trivially destructible
 * values on the stack.
 *
 * @tparam T The type of values to be stored.
 * @tparam CAPACITY The capacity of elements in the array.
 */
template <typename T, int CAPACITY>
class StaticList {
public:
    using TVal = T;
    using Iterator = TVal*;
    using ConstIterator = const TVal*;

    inline void insert(const TVal& val, int index) {
        for (int i = m_Size; i > index; --i) {
            m_Arr[i] = m_Arr[i - 1];
        }
        m_Arr[index] = val;
        m_Size++;
    }

    inline void add(const TVal& val) {
        LUNA_ASSERT(m_Size < capacity(), "Cannot add beyond capacity.");

        m_Arr[m_Size] = val;
        m_Size++;
    }

    inline void removeLast() {
        LUNA_ASSERT(m_Size > 0, "Trying to remove when empty.");
        m_Size--;
    }

    inline void removeAt(int index) {
        LUNA_ASSERT(m_Size > 0, "Trying to remove when empty.");
        m_Size--;
        for (int i = index; i < m_Size; ++i) {
            m_Arr[i] = m_Arr[i + 1];
        }
    }

    inline int indexOf(const TVal& val) const {
        for (int i = 0; i < m_Size; ++i) {
            if (m_Arr[i] == val) {
                return i;
            }
        }

        return -1;
    }

    inline bool contains(const TVal& val) const {
        return indexOf(val) != -1;
    }

    inline bool remove(const TVal& val) {
        int idx = indexOf(val);

        if (idx != -1) {
            removeAt(idx);
            return true;
        }

        return false;
    }

    inline void clear() {
        m_Size = 0;
    }

    inline TVal& operator[](int index) { return m_Arr[index]; }
    inline const TVal& operator[](int index) const { return m_Arr[index]; }

    inline int size() const { return m_Size; }
    inline constexpr int capacity() const { return CAPACITY; }

    //
    // Iterators
    //

    inline Iterator begin() { return &m_Arr[0]; }
    inline Iterator end() { return &m_Arr[m_Size]; }
    inline Iterator cbegin() const { return &m_Arr[0]; }
    inline Iterator cend() const { return &m_Arr[m_Size]; }

private:
    TVal m_Arr[CAPACITY];
    int m_Size = 0;
};

template <typename T, int CAPACITY>
std::ostream& operator<<(std::ostream& stream, const StaticList<T, CAPACITY>& l) {
    stream << "['";
    for (int i = 0; i < l.size() - 1; ++i) {
        stream << l[i] << "', '";
    }
    if (l.size() > 0) {
        stream << l[l.size() - 1];
    }
    stream << "']";
    return stream;
}

}

#endif //LUNA_STATICLIST_H
