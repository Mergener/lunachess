#include "transpositiontable.h"

namespace lunachess::ai {

bool TranspositionTable::maybeAdd(const Entry& entry) {
    Bucket& b = getBucket(entry.zobristKey);
    if (!b.valid) {
        // New tt entry
        b.entry = entry;
        b.valid = true;
        m_Count++;
        return true;
    }

    if (b.entry.depth == entry.depth &&
        entry.type    == TranspositionTable::EXACT &&
        b.entry.type  != TranspositionTable::EXACT) {
        // Replace since we now have an exact score
        b.entry = entry;
        b.valid = true;
        return true;
    }

    if (b.entry.depth <= entry.depth) {
        // New one has a higher depth, replace
        b.entry = entry;
        b.valid = true;
        return true;
    }
    return false;
}

}