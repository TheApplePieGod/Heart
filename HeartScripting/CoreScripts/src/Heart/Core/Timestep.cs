using System.Runtime.CompilerServices;

namespace Heart.Core
{
    public struct Timestep
    {
        public Timestep(double time)
        {
            _time = time;
        }

        public double StepSeconds
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _time / 1000.0;
        }

        public double StepMilliseconds
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _time;
        }

        public double StepNanoseconds
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _time * 1000000.0;
        }

        private double _time = 0;
    }
}
