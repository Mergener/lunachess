#include "transpositiontable.h"

namespace lunachess::ai {

TranspositionTable::TranspositionTable() {
}

void TranspositionTable::remove(ui64 posKey) {
    m_Entries.erase(posKey);
}

bool TranspositionTable::maybeAdd(const Entry& entry) {
    auto it = m_Entries.find(entry.zobristKey);
    if (it == m_Entries.cend()) {
        m_Entries[entry.zobristKey] = entry;
        return true;
    }
    else if (it->second.depth < entry.depth) {
        m_Entries[entry.zobristKey] = entry;
        return true;
    }
    return false;
}

bool TranspositionTable::tryGet(ui64 posKey, Entry& entry) const {
    auto it = m_Entries.find(posKey);
    if (it == m_Entries.cend()) {
        return false;
    }

    entry = it->second;

    return true;
}

}