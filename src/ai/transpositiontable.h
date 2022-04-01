#ifndef LUNA_AI_TRANSPOSITIONTABLE_H
#define LUNA_AI_TRANSPOSITIONTABLE_H

#include <unordered_map>

#include "../position.h"
#include "../types.h"
#include "../zobrist.h"

namespace lunachess::ai {

class TranspositionTable {
public:
    enum EntryType {
        // For PV-Nodes
        EXACT,

        // For Cut-Nodes
        LOWERBOUND,

        // For All-Nodes
        UPPERBOUND
    };

    struct Entry {
        ui64 zobristKey;
        EntryType type;
        int score;
        int depth;
        Move move;
        int staticEval;
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
    bool maybeAdd(const Entry& entry);

    inline bool tryGet(const Position& pos, Entry& entry) const {
        return tryGet(pos.getZobrist(), entry);
    }

    inline void reserve(int size) {
        m_Entries.reserve(size);
    }

    bool tryGet(ui64 posKey, Entry& entry) const;

    inline void remove(const Position& pos) {
        remove(pos.getZobrist());
    }

    void remove(ui64 posKey);

    inline void clear() {
        m_Entries.clear();
    }

    inline int count() const {
        return m_Entries.size();
    }

    TranspositionTable();

private:
    Container m_Entries;
};

}

#endif // LUNA_AI_TRANSPOSITIONTABLE_H
