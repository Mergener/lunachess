#include "transpositiontable.h"

namespace lunachess::ai {

bool TranspositionTable::maybeAdd(const Entry& entry) {
    Bucket& b = getBucket(entry.zobristKey);
    if (!b.valid) {
        b.entry = entry;
        b.valid = true;
        return true;
    }
    //// We had a valid bucket, check if same key
    //if (b.entry.zobristKey != entry.zobristKey) {
    //    // Different keys, never replace
    //    return false;
    //    //b.entry = entry;
    //    //b.valid = true;
    //    //return true;
    //}
    if (b.entry.depth == entry.depth &&
        entry.type == TranspositionTable::EXACT &&
        b.entry.type != TranspositionTable::EXACT) {
        b.entry = entry;
        b.valid = true;
        return true;
    }

    if (b.entry.depth < entry.depth) {
        // New one has a higher depth, replace
        b.entry = entry;
        b.valid = true;
        return true;
    }
    return false;
}

}