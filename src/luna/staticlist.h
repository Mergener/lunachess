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
template <typename T, i32 CAPACITY>
class StaticList {
public:
    inline static constexpr i32 MAX_ELEMS = CAPACITY;

    using TVal = T;
    using Iterator = TVal*;
    using ConstIterator = const TVal*;

    inline void insert(const TVal& val, i32 index) {
        LUNA_ASSERT(index <= m_Size && index >= 0, "Index out of bounds.");
        for (i32 i = m_Size; i > index; --i) {
            (*this)[i] = (*this)[i - 1];
        }
        (*this)[index] = val;
        m_Size++;
    }

    inline void insert(const TVal& val, Iterator it) {
        i32 index = it - begin();
        insert(val, index);
    }

    inline void add(const TVal& val) {
        LUNA_ASSERT(m_Size < capacity(), "Cannot add beyond capacity.");

        (*this)[m_Size] = val;
        m_Size++;
    }

    template <typename TIter>
    inline void addRange(TIter srcBegin, TIter srcEnd) {
        i32 delta = srcEnd - srcBegin;
        LUNA_ASSERT(delta + m_Size <= capacity(), "Cannot add range beyond capacity.");
        std::copy(srcBegin, srcEnd, end());
        m_Size += delta;
    }

    inline void removeLast() {
        LUNA_ASSERT(m_Size > 0, "Trying to remove when empty.");

        if constexpr (!std::is_trivially_destructible_v<TVal>) {
            std::destroy_at((*this)[m_Size - 1]);
        }

        m_Size--;
    }

    inline void removeAt(i32 index) {
        LUNA_ASSERT(m_Size > 0, "Trying to remove when empty.");
        LUNA_ASSERT(index < m_Size && index >= 0, "Index out of bounds.");

        if constexpr (!std::is_trivially_destructible_v<TVal>) {
            std::destroy_at((*this)[index]);
        }
        m_Size--;
        for (i32 i = index; i < m_Size; ++i) {
            (*this)[i] = (*this)[i + 1];
        }
    }

    inline i32 indexOf(const TVal& val) const {
        for (i32 i = 0; i < m_Size; ++i) {
            if ((*this)[i] == val) {
                return i;
            }
        }

        return -1;
    }

    inline bool contains(const TVal& val) const {
        return indexOf(val) != -1;
    }

    inline bool remove(const TVal& val) {
        i32 idx = indexOf(val);

        if (idx != -1) {
            removeAt(idx);
            return true;
        }

        return false;
    }

    inline void clear() {
        if constexpr (!std::is_trivially_destructible_v<TVal>) {
            for (auto& el: *this) {
                std::destroy_at(el);
            }
        }
        m_Size = 0;
    }

    inline TVal& operator[](i32 index) { return *std::launder(reinterpret_cast<T*>(&m_Arr[index])); }
    inline const TVal& operator[](i32 index) const { return *std::launder(reinterpret_cast<const T*>(&m_Arr[index])); }

    inline i32 size() const { return m_Size; }
    inline constexpr i32 capacity() const { return CAPACITY; }

    //
    // Iterators
    //

    inline Iterator begin() { return &((*this)[0]); }
    inline Iterator end() { return &((*this)[m_Size]); }
    inline Iterator cbegin() const { return &((*this)[0]); }
    inline Iterator cend() const { return &((*this)[m_Size]); }

private:
    std::aligned_storage_t<sizeof(TVal), alignof(TVal)> m_Arr[CAPACITY];
    i32 m_Size = 0;
};

template <typename T, i32 CAPACITY>
std::ostream& operator<<(std::ostream& stream, const StaticList<T, CAPACITY>& l) {
    stream << "['";
    for (i32 i = 0; i < l.size() - 1; ++i) {
        stream << l[i] << "', '";
    }
    if (l.size() > 0) {
        stream << l[l.size() - 1];
    }
    stream << "']";
    return stream;
}

} // lunachess

#endif //LUNA_STATICLIST_H
