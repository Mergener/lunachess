#ifndef LUNA_HOTMAP_H
#define LUNA_HOTMAP_H

#include <iostream>
#include <cstring>
#include <array>
#include <initializer_list>

#include <nlohmann/json.hpp>

#include "../types.h"

namespace lunachess::ai {

class KingSquareHotmapGroup;

class Hotmap {
public:
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Hotmap, m_Values);

    static KingSquareHotmapGroup defaultMiddlegameMaps[PT_COUNT - 1];
    static KingSquareHotmapGroup defaultEndgameMaps[PT_COUNT - 1];
    static Hotmap defaultKingMgHotmap;
    static Hotmap defaultKingEgHotmap;

    static void initializeHotmaps();

    inline int getValue(Square s, Color pov) const {
        return m_Values[squareToIdx(s, pov)];
    }

    inline void setValue(Square s, Color pov, int value) {
        m_Values[squareToIdx(s, pov)] = value;
    }

    inline Hotmap() {
        std::fill(m_Values.begin(), m_Values.end(), 0);
    }

    inline Hotmap(const std::array<short, SQ_COUNT>& values)
        : m_Values(values) {
    }

    inline Hotmap(const std::initializer_list<short>& values) {
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
        Square ret;
        if (c == CL_WHITE) {
            int file = getFile(s);
            int rank = getRank(s);

            rank = 7 - rank;
            ret = rank * 8 + file;
        }
        else {
            ret = s;
        }
        return ret;
    }
};

class KingSquareHotmapGroup {
public:
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(KingSquareHotmapGroup, m_Hotmaps);

    inline Hotmap& getHotmap(Square kingSquare) {
        BoardFile file = static_cast<int>(getFile(kingSquare));
        if (file >= FL_E) {
            file = FL_H - file;
        }

        BoardRank rank = getRank(kingSquare);

        // Invert ranks and files to get numbers less than 32
        Square idx = getSquare(rank, file);

        return m_Hotmaps[idx];
    }

    inline const Hotmap& getHotmap(Square kingSquare) const {
        return const_cast<KingSquareHotmapGroup*>(this)->getHotmap(kingSquare);
    }

    KingSquareHotmapGroup() = default;
    inline KingSquareHotmapGroup(const Hotmap& hotmap) {
        std::fill(m_Hotmaps.begin(), m_Hotmaps.end(), hotmap);
    }

    inline KingSquareHotmapGroup& operator=(const Hotmap& hotmap) {
        std::fill(m_Hotmaps.begin(), m_Hotmaps.end(), hotmap);
        return *this;
    }

    inline auto begin() { return m_Hotmaps.begin(); }
    inline auto end() { return m_Hotmaps.end(); }
    inline auto cbegin() const { return m_Hotmaps.cbegin(); }
    inline auto cend() const { return m_Hotmaps.cend(); }

private:
    std::array<Hotmap, 32> m_Hotmaps;
};

}

#endif // LUNA_HOTMAP_H