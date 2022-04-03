#ifndef LUNA_AI_TIMEMANAGER_H
#define LUNA_AI_TIMEMANAGER_H

#include "../clock.h"
#include "../types.h"

namespace lunachess::ai {

class TimeManager {
public:
    void start(const TimeControl& tc);
    bool timeIsUp() const;

private:
    Clock m_Clock;
    TimePoint m_Start;
    TimeControl m_Tc;
    bool m_ForceStop = false;
};

}

#endif // LUNA_AI_TIMEMANAGER_H
