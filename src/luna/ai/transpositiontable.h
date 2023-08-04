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
        ui64 zobristKey = 0;
        Move move  = MOVE_INVALID;
        i32  score = 0;
        i32  staticEval = 0;
        ui8  depth = 0;
        EntryType type = {};
    };

private:

    struct Bucket {
        Entry entry;
        bool valid = false;
    };

public:
    inline size_t getCapacity() const {
        return m_Capacity;
    }

    inline size_t getCount() const {
        return m_Count;
    }

    /**
        Adds a given entry to the transposition table, except if
        an entry for the same position with higher depth exists.
    */
    bool maybeAdd(const Entry& entry);

    /**
     * Probes the transposition table for an entry.
     *
     * @param posKey The zobrist key of the position to probe.
     * @param entry Reference to the entry object to receive the data.
     * @return True if an entry with a matching key was found, false otherwise.
     */
    inline bool probe(ui64 posKey, Entry& entry) const {
        Bucket& bucket = getBucket(posKey);
        if (bucket.valid && bucket.entry.zobristKey == posKey) {
            entry = bucket.entry;
            return true;
        }
        return false;
    }

    inline bool probe(const Position& pos, Entry& entry) const {
        return probe(pos.getZobrist(), entry);
    }

    inline void remove(ui64 posKey) {
        getBucket(posKey).valid = false;
    }

    inline void remove(const Position& pos) {
        remove(pos.getZobrist());
    }

    inline void clear() {
        // Simply resize to the same capacity.
        resize(m_Capacity * sizeof(Bucket));
    }

    /**
     * Resizes the transposition table. Deletes all entries.
     */
    inline void resize(size_t hashSizeBytes) {
        if (m_Buckets != nullptr) {
            std::free(m_Buckets);
        }

        m_Count    = 0;
        m_Capacity = hashSizeBytes / sizeof(Bucket);
        m_Buckets  = static_cast<Bucket*>(std::calloc(m_Capacity, sizeof(Bucket)));
        if (m_Buckets == nullptr) {
            throw std::bad_alloc();
        }
    }

    inline TranspositionTable(size_t hashSizeBytes = DEFAULT_SIZE_MB * 1024 * 1024) {
        resize(hashSizeBytes);
    }

    inline ~TranspositionTable() {
        if (m_Buckets != nullptr) {
            std::free(m_Buckets);
        }
    }

private:
    Bucket* m_Buckets  = nullptr;
    size_t  m_Capacity = 0;
    size_t  m_Count    = 0;

    inline Bucket& getBucket(ui64 key) const {
        return m_Buckets[key % m_Capacity];
    }
};

}

#endif // LUNA_AI_TRANSPOSITIONTABLE_H
