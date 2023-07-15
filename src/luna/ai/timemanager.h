#ifndef LUNA_AI_TIMEMANAGER_H
#define LUNA_AI_TIMEMANAGER_H

#include "../clock.h"
#include "../move.h"
#include "../types.h"

namespace lunachess::ai {

class SearchResults;

class TimeManager {
public:
    void start(const TimeControl& tc);
    void onNewDepth(const SearchResults& res);

    bool timeIsUp() const;

private:
    TimePoint m_Start;
    i64 m_TargetTime;
    i64 m_OriginalTargetTime;
    TimeControl m_Tc;
    Move m_BestItMove;
    int m_ItMoveReps;
};

}

#endif // LUNA_AI_TIMEMANAGER_H
