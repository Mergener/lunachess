#ifndef LUNA_AI_TRANSPOSITIONTABLE_H
#define LUNA_AI_TRANSPOSITIONTABLE_H

#include <cstring>

#include "../position.h"
#include "../types.h"
#include "../zobrist.h"

namespace lunachess::ai {

class TranspositionTable {

public:
    static constexpr size_t DEFAULT_SIZE_MB = 32;

    enum EntryType : ui8 {
        // For PV-Nodes
        EXACT,

        // For Cut-Nodes
        LOWERBOUND,

        // For All-Nodes
        UPPERBOUND
    };

    struct Entry {
        ui64 zobristKey;
        Move move;
        i32 score;
        i32 staticEval;
        ui8 depth;
        EntryType type;
    };

private:

    struct Bucket {
        Entry entry;
        bool valid;
    };

public:
    /**
        Adds a given entry to the transposition table, except if
        an entry for the same position with higher depth exists.
    */
    bool maybeAdd(const Entry& entry);

    inline bool tryGet(ui64 posKey, Entry& entry) const {
        Bucket& bucket = getBucket(posKey);
        if (bucket.valid && bucket.entry.zobristKey == posKey) {
            entry = bucket.entry;
            return true;
        }
        return false;
    }

    inline bool tryGet(const Position& pos, Entry& entry) const {
        return tryGet(pos.getZobrist(), entry);
    }

    inline void remove(ui64 posKey) {
        getBucket(posKey).valid = false;
    }

    inline void remove(const Position& pos) {
        remove(pos.getZobrist());
    }

    inline void clear() {
        std::memset(m_Buckets, 0, m_Capacity * sizeof(Bucket));
    }

    /**
     * Resizes the transposition table. Deletes all entries.
     */
    inline void resize(size_t hashSizeBytes) {
        if (m_Buckets != nullptr) {
            delete m_Buckets;
        }

        m_Capacity = hashSizeBytes / sizeof(Bucket);
        m_Buckets = new Bucket[m_Capacity];
        std::memset(m_Buckets, 0, m_Capacity * sizeof(Bucket));
    }

    inline TranspositionTable(size_t hashSizeBytes = DEFAULT_SIZE_MB * 1024 * 1024) {
        resize(hashSizeBytes);
    }

private:
    Bucket* m_Buckets = nullptr;
    size_t m_Capacity = 0;

    inline Bucket& getBucket(ui64 key) const {
        return m_Buckets[key % m_Capacity];
    }
};

}

#endif // LUNA_AI_TRANSPOSITIONTABLE_H
