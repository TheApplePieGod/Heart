#pragma once

namespace Heart
{
    class Timestep
    {
    public:
        Timestep(double timeInSeconds)
            : m_FrameTime(timeInSeconds)
        {}

        inline double StepSeconds() const { return m_FrameTime; }
        inline double StepMilliseconds() const { return m_FrameTime * 1000.0; }

    private:
        double m_FrameTime; // in seconds
    };
}