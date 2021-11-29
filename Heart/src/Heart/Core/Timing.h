#pragma once

namespace Heart
{
    class Timer
    {
    public:
        Timer(const std::string& name, bool shouldLog = true)
            : m_Name(name), m_ShouldLog(shouldLog)
        { Reset(); }

        Timer()
        { Reset(); }

        ~Timer()
        {
            if (m_ShouldLog)
                Log();
        }

        inline void Log()
        {
            if (!m_Name.empty())
                HE_ENGINE_LOG_INFO("{0} took {1} ms", m_Name, static_cast<u32>(ElapsedMilliseconds()));
        }

        inline void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

        inline void SetName(const std::string& newName) { m_Name = newName; } 

        inline double ElapsedSeconds()
        {
            return ElapsedNanoseconds() * 0.000000001;
        }

        inline double ElapsedMilliseconds()
        {
            return ElapsedNanoseconds() * 0.000001;
        }

        inline double ElapsedNanoseconds()
        {
            auto stop = std::chrono::high_resolution_clock::now();
            return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(stop - m_Start).count());
        }

    protected:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
        std::string m_Name;
        bool m_ShouldLog;
    };

    class AggregateTimer : public Timer
    {
    public:
        AggregateTimer(const std::string& name)
            : Timer(name, false)
        {}

        ~AggregateTimer()
        {
            std::unique_lock lock(s_Mutex);
            s_AggregateTimes[m_Name] += ElapsedMilliseconds();
        }

        inline static double GetAggregateTime(const std::string& name)
        {
            std::shared_lock lock(s_Mutex);
            if (s_AggregateTimes.find(name) != s_AggregateTimes.end())
                return s_AggregateTimes[name];
            else
                return 0;
        }

        inline static void ResetAggregateTime(const std::string& name)
        {
            std::unique_lock lock(s_Mutex);
            if (s_AggregateTimes.find(name) != s_AggregateTimes.end())
                s_AggregateTimes[name] = 0;
        }

        inline static const std::unordered_map<std::string, double>& GetTimeMap() { return s_AggregateTimesLastFrame; }
        inline static void ClearTimeMap() { std::unique_lock lock(s_Mutex); s_AggregateTimes.clear(); }

        inline static void ResetAggregateTimes()
        {
            std::unique_lock lock(s_Mutex);
            s_AggregateTimesLastFrame = s_AggregateTimes;
            for (auto& pair : s_AggregateTimes)
                pair.second = 0;
        }

    private:
        static std::unordered_map<std::string, double> s_AggregateTimes; // stored in millis
        static std::unordered_map<std::string, double> s_AggregateTimesLastFrame;
        static std::shared_mutex s_Mutex;
    };
}