#include "timemanager.h"

#include "search.h"

namespace lunachess::ai {

void TimeManager::start(const TimeControl& tc) {
    m_Tc = tc;
    m_Start = Clock::now();

    // Calculate target time
    switch (m_Tc.mode) {
        case TC_MOVETIME:
            m_TargetTime = tc.time - 50;
            break;

        case TC_FISCHER:
            m_TargetTime = tc.time / 18;
            break;

        default:
            break;
    }
}

void TimeManager::onNewDepth(const SearchResults& res) {
    if (m_Tc.mode != TC_FISCHER) {
        // Nothing to do
        return;
    }

    // We have a target time that we want to reach.
    // In 'movetime' modes, we want to use it as much as possible,
    // since for every move we will have the same time to
    // spend again.
    // In Fischer time controls, however, if we think we are not going
    // to finish the search in our speculated target time, it is better
    // to not go further onto the search.

    constexpr int EXPECTED_BRANCH_FACTOR = 5;
    i64 depthTime = res.getCurrDepthTime();
    i64 totalTime = res.getSearchTime();

    if (totalTime + (depthTime * EXPECTED_BRANCH_FACTOR) >= m_TargetTime) {
        // Setting the target time to 0 will make the next call to
        // timeIsUp() return true.
        m_TargetTime = 0;
    }
}

bool TimeManager::timeIsUp() const {
    if (m_Tc.mode == TC_INFINITE) {
        return false;
    }

    i64 delta = deltaMs(Clock::now(), m_Start);
    return delta >= m_TargetTime;
}

}