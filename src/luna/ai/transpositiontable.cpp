#include "transpositiontable.h"

namespace lunachess::ai {

bool TranspositionTable::maybeAdd(const Entry& entry) {
    Bucket& b = getBucket(entry.zobristKey);
    if (!b.isValid()) {
        // New tt entry
        b.replace(entry, m_Gen);
        m_Count++;
        return true;
    }

    if (b.getGeneration() != m_Gen) {
        // Always replace older generation buckets for new ones
        b.replace(entry, m_Gen);
        return true;
    }

    if (b.getEntry().depth <= entry.depth &&
        entry.type         == TranspositionTable::EXACT &&
        b.getEntry().type  != TranspositionTable::EXACT) {
        // Replace since we now have an exact score
        b.replace(entry, m_Gen);
        return true;
    }

    if (b.getEntry().depth <= entry.depth) {
        // New one has a higher depth, replace
        b.replace(entry, m_Gen);
        return true;
    }
    return false;
}

}