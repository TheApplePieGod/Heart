using Heart.NativeInterop;
using Heart.Scene;
using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Task
{
    public interface ISchedulable
    {
        public void Run();
        public void ScheduleParallel();
    }

    public class Schedulable : ISchedulable
    {
        Action<nuint> _runFunc;
        Func<nuint, bool> _checkFunc;
        nuint _count;

        public Schedulable(nuint count, Action<nuint> func, Func<nuint, bool> check = null)
        {
            _count = count;
            _runFunc = func;
            _checkFunc = check;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void RunUnchecked()
        {
            for (nuint i = 0; i < _count; i++)
                _runFunc(i);
        }

        public void Run()
        {
            if (_checkFunc == null)
            {
                RunUnchecked();
                return;
            }

            for (nuint i = 0; i < _count; i++)
                if (_checkFunc(i))
                    _runFunc(i);
        }

        public void ScheduleParallel()
        {

        }
    }

    public class SchedulableIter : ISchedulable
    {
        public delegate void RunFn(nuint val);
        internal delegate InteropBool GetNextIterFn(out nuint outVal);

        IEnumerable<nuint> _iterFunc;
        RunFn _runFunc;
        Func<nuint, bool> _checkFunc;

        public SchedulableIter(IEnumerable<nuint> iter, RunFn runFunc, Func<nuint, bool> check = null)
        {
            _iterFunc = iter;
            _runFunc = runFunc;
            _checkFunc = check;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void RunUnchecked()
        {
            foreach (nuint val in _iterFunc)
                _runFunc(val);
        }

        public void Run()
        {
            if (_checkFunc == null)
            {
                RunUnchecked();
                return;
            }

            foreach (nuint val in _iterFunc)
                if (_checkFunc(val))
                    _runFunc(val);
        }

        public void ScheduleParallel()
        {
            var enumerator = _iterFunc.GetEnumerator();
            GetNextIterFn callback = new GetNextIterFn((out nuint outVal) => {
                bool result = enumerator.MoveNext();
                outVal = enumerator.Current;
                return NativeMarshal.BoolToInteropBool(result);
            });

            Native_SchedulableIter_Schedule(callback, _runFunc);
        }

        [DllImport("__Internal")]
        internal static extern void Native_SchedulableIter_Schedule(GetNextIterFn getNext, RunFn run);
    }
}
