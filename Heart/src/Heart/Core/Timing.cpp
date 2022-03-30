#include "hepch.h"
#include "Timing.h"

namespace Heart
{
    std::map<std::string, double> AggregateTimer::s_AggregateTimes;
    std::map<std::string, double> AggregateTimer::s_AggregateTimesLastFrame;
    std::shared_mutex AggregateTimer::s_CurrentMutex;
}