#pragma once

#include <chrono>

namespace Heart
{
    class Timer
    {
    public:
        Timer(const std::string& name)
            : m_Name(name)
        { Reset(); }

        ~Timer()
        {
            if (!m_Name.empty())
                HE_ENGINE_LOG_INFO("{0} took {1} ms", m_Name, static_cast<u32>(ElapsedMilliseconds()));
        }

        inline void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

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

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
        std::string m_Name;
    };
}