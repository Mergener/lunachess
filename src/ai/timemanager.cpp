#include "timemanager.h"

namespace lunachess::ai {

void TimeManager::start(const TimeControl& tc) {
    m_Tc = tc;
    m_Start = m_Clock.now();
}

bool TimeManager::timeIsUp() const {
    i64 delta;
    switch (m_Tc.mode) {
        case TC_INFINITE:
            return false;

        case TC_MOVETIME:
            delta = deltaMs(m_Clock.now(), m_Start);
            return delta >= (m_Tc.time  - 50);

        case TC_FISCHER:
            delta = deltaMs(m_Clock.now(), m_Start);
            return delta >= (m_Tc.time / 20);
    }
    return true;
}

}