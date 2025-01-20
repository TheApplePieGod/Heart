#pragma once

#include "Heart/Container/HString8.h"
#include "Heart/Container/HVector.hpp"

namespace Heart
{
    class Timer
    {
    public:
        /**
         * @brief Default constructor.
         *
         * @param name The debug name for the timer.
         * @param shouldLog Whether or not the timer should log when destructed.
         */
        Timer(const HString8& name, bool shouldLog = true)
            : m_Name(name), m_ShouldLog(shouldLog)
        { Reset(); }

        /*! @brief Default constructor. */
        Timer()
        { Reset(); }

        /*! @brief Default destructor. */
        ~Timer()
        {
            if (m_ShouldLog)
                Log();
        }

        /*! @brief Log the timer's data (name and elapsed ms). */
        inline void Log()
        {
            if (!m_Name.IsEmpty())
                HE_ENGINE_LOG_INFO("{0} took {1} ms", m_Name.Data(), static_cast<u32>(ElapsedMilliseconds()));
        }

        /*! @brief Reset the timer to zero. */
        inline void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

        /**
         * @brief Change the debug name of the timer.
         *
         * @param newName The new name of the timer.
         */
        inline void SetName(const HString8& newName) { m_Name = newName; } 

        /*! @brief Get the elapsed time of the timer in seconds. */
        inline double ElapsedSeconds()
        {
            return ElapsedNanoseconds() * 0.000000001;
        }

        /*! @brief Get the elapsed time of the timer in milliseconds. */
        inline double ElapsedMilliseconds()
        {
            return ElapsedNanoseconds() * 0.000001;
        }

        /*! @brief Get the elapsed time of the timer in nanoseconds. */
        inline double ElapsedNanoseconds()
        {
            auto stop = std::chrono::high_resolution_clock::now();
            return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(stop - m_Start).count());
        }

    protected:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
        HString8 m_Name;
        bool m_ShouldLog;
    };

    class AggregateTimer : public Timer
    {
    public:
        /**
         * @brief Default constructor.
         *
         * @param name The name/id associated with this timer.
         */
        AggregateTimer(const HString8& name)
            : Timer(name, false)
        {}

        /*! @brief Default destructor. */
        ~AggregateTimer()
        {
            Finish();
        }
        
        void Finish()
        {
            if (m_Finished) return;
            m_Finished = true;

            // For now, throw out insignificant samples. Need to figure out how
            // to do a copy constructor without calling the destructor
            double ms = ElapsedMilliseconds();
            if (ms < 0.01)
                return;

            std::unique_lock lock(s_CurrentMutex);
            auto& samples = s_AggregateTimes[m_Name];
            samples.Insert(ms, 0);
        }
        
        inline static constexpr u32 MaxSamples = 5;
        
    public:
        /**
         * @brief Get the globally accumulated time for a specific timer id in milliseconds.
         *
         * @param name The name/id of the timer.
         * @return The time in milliseconds or zero if the id is invalid.
         */
        static double GetAggregateTime(const HString8& name)
        {
            std::shared_lock lock(s_CurrentMutex);
            if (s_AggregateTimes.find(name) == s_AggregateTimes.end()) return 0;
            
            double average = 0.0;
            const auto& samples = s_AggregateTimes[name];
            if (samples.IsEmpty())
                return average;

            for (double val : samples)
                average += val;

            return average / samples.Count();
        }

        /**
         * @brief Reset the current globally accumulated time for a specific timer id to zero.
         *
         * @param name The name/id of the timer.
         */
        static void ResetAggregateTime(const HString8& name)
        {
            std::unique_lock lock(s_CurrentMutex);
            if (s_AggregateTimes.find(name) != s_AggregateTimes.end())
                s_AggregateTimes[name].Clear();
        }

        /*! @brief Store the current aggregate times for retrieval and prepare for next frame. */
        static void EndFrame()
        {
            std::unique_lock lock(s_CurrentMutex);
            for (auto& pair : s_AggregateTimes)
                while (pair.second.Count() > MaxSamples) // Remove oldest samples
                    pair.second.Pop();
        }

        /*! @brief Get the map containing all timer ids and aggregate times from the last frame. */
        inline static const std::map<HString8, HVector<double>>& GetTimeMap() { return s_AggregateTimes; }

        /*! @brief Clear all current stored timer ids and aggregate times. */
        inline static void ClearTimeMap() { std::unique_lock lock(s_CurrentMutex); s_AggregateTimes.clear(); }

    private:
        inline static std::map<HString8, HVector<double>> s_AggregateTimes; // stored in millis
        inline static std::shared_mutex s_CurrentMutex;
        
    private:
        bool m_Finished = false;
    };
}
