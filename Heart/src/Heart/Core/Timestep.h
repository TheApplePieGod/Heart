#pragma once

namespace Heart
{
    class Timestep
    {
    public:
        /**
         * @brief Default constructor.
         * 
         * @param time The time this timestep represents in milliseconds. 
         */
        Timestep(double time)
            : m_Time(time)
        {}

        /*! @brief Default constructor. */
        Timestep() = default;

        /*! @brief Get the time represented by this timestep in seconds. */
        inline double StepSeconds() const { return m_Time / 1000.0; }

        /*! @brief Get the time represented by this timestep in milliseconds. */
        inline double StepMilliseconds() const { return m_Time; }

        /*! @brief Get the time represented by this timestep in nanoseconds. */
        inline double StepNanoseconds() const { return m_Time * 1000000.0; }

    private:
        double m_Time = 0.0; // milliseconds
    };
}