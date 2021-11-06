#include "htpch.h"
#include "Timing.h"

namespace Heart
{
    std::unordered_map<std::string, u32> AggregateTimer::s_AggregateTimes = {};
    std::shared_mutex AggregateTimer::s_Mutex;
}