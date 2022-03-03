#pragma once

namespace Heart
{
    class Timestep
    {
    public:
        /* TODO: CHANGE FRAMETIME AND STOP USING SECONDS */
        Timestep(double timeInSeconds)
            : m_FrameTime(timeInSeconds)
        {}

        Timestep() = default;

        inline double StepSeconds() const { return m_FrameTime; }
        inline double StepMilliseconds() const { return m_FrameTime * 1000.0; }

    private:
        double m_FrameTime = 0.0; // in seconds
    };
}