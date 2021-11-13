#include "htpch.h"
#include "Timing.h"

namespace Heart
{
    std::unordered_map<std::string, double> AggregateTimer::s_AggregateTimes = {};
    std::shared_mutex AggregateTimer::s_Mutex;
}