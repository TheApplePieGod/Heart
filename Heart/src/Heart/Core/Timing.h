#pragma once

#include "Heart/Container/HString8.h"

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
        Timer(const HStringView8& name, bool shouldLog = true)
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
        inline void SetName(const HStringView8& newName) { m_Name = newName; } 

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
        AggregateTimer(const HStringView8& name)
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
            std::unique_lock lock(s_CurrentMutex);
            s_AggregateTimes[m_Name] += ElapsedMilliseconds();
        }
        
    public:
        /**
         * @brief Get the globally accumulated time for a specific timer id in milliseconds.
         *
         * @param name The name/id of the timer.
         * @param current True if the time should be retrieved from the current frame and false for the previous frame.
         * @return The time in milliseconds or zero if the id is invalid.
         */
        static double GetAggregateTime(const HStringView8& name, bool current)
        {
            if (current)
            {
                std::shared_lock lock(s_CurrentMutex);
                if (s_AggregateTimes.find(name) != s_AggregateTimes.end())
                    return s_AggregateTimes[name];
                else
                    return 0;
            }
            else {
                if (s_AggregateTimesLastFrame.find(name) != s_AggregateTimesLastFrame.end())
                    return s_AggregateTimesLastFrame[name];
                else
                    return 0;
            }
        }

        /**
         * @brief Reset the current globally accumulated time for a specific timer id to zero.
         *
         * @param name The name/id of the timer.
         */
        static void ResetAggregateTime(const HStringView8& name)
        {
            std::unique_lock lock(s_CurrentMutex);
            if (s_AggregateTimes.find(name) != s_AggregateTimes.end())
                s_AggregateTimes[name] = 0;
        }

        /*! @brief Store the current aggregate times for retrieval and prepare for next frame. */
        static void EndFrame()
        {
            std::unique_lock lock(s_CurrentMutex);
            s_AggregateTimesLastFrame = s_AggregateTimes;
            s_AggregateTimes.clear();
        }

        /*! @brief Get the map containing all timer ids and aggregate times from the last frame. */
        inline static const std::map<HString8, double>& GetTimeMap() { return s_AggregateTimesLastFrame; }

        /*! @brief Clear all current stored timer ids and aggregate times. */
        inline static void ClearTimeMap() { std::unique_lock lock(s_CurrentMutex); s_AggregateTimes.clear(); }

    private:
        inline static std::map<HString8, double> s_AggregateTimes; // stored in millis
        inline static std::map<HString8, double> s_AggregateTimesLastFrame;
        inline static std::shared_mutex s_CurrentMutex;
        
    private:
        bool m_Finished = false;
    };
}
