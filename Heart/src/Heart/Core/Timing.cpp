#include "hepch.h"
#include "Timing.h"

namespace Heart
{
    std::unordered_map<std::string, double> AggregateTimer::s_AggregateTimes;
    std::unordered_map<std::string, double> AggregateTimer::s_AggregateTimesLastFrame;
    std::shared_mutex AggregateTimer::s_Mutex;
}