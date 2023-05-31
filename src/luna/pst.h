#ifndef LUNA_PST_H
#define LUNA_PST_H

#include <iomanip>

#include <nlohmann/json.hpp>

#include "types.h"

namespace lunachess {

class PieceSquareTable {
public:
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PieceSquareTable, m_Values);

    inline short& valueAt(Square s, Color pov) {
        return m_Values[squareToIdx(s, pov)];
    }

    inline short valueAt(Square s, Color pov) const {
        return m_Values[squareToIdx(s, pov)];
    }

    inline PieceSquareTable() noexcept {
        std::fill(m_Values.begin(), m_Values.end(), 0);
    }

    inline PieceSquareTable(const std::array<short, SQ_COUNT>& values)
            : m_Values(values) {
    }

    inline PieceSquareTable(const std::initializer_list<short>& values) {
        size_t i;
        auto it = values.begin();
        for (i = 0; i < values.size() && i < SQ_COUNT; ++i) {
            m_Values[i] = *it;
            ++it;
        }

        std::fill(m_Values.begin() + i, m_Values.end(), 0);
    }

    using Iterator = short*;
    using ConstIterator = const short*;

    inline Iterator begin() { return &m_Values[0]; }
    inline Iterator end() { return &m_Values[SQ_COUNT]; }
    inline ConstIterator cbegin() const { return &m_Values[0]; }
    inline ConstIterator cend() const { return &m_Values[SQ_COUNT]; }

private:
    std::array<short, SQ_COUNT> m_Values;

    inline static Square squareToIdx(Square s, Color c) {
        return c == CL_WHITE ? mirrorVertically(s) : s;
    }
};

std::ostream& operator<<(std::ostream& stream, const PieceSquareTable& hotmap);

}

#endif // LUNA_PST_H
