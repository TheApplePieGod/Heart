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
        public delegate void RunFn(nuint val);

        private Action _completeFunc;
        private RunFn _runFunc;
        private Func<nuint, bool> _checkFunc;
        private nuint _count;

        public Schedulable(
            nuint count,
            RunFn runFunc,
            Func<nuint, bool> checkFunc = null,
            Action completeFunc = null
        )
        {
            _count = count;
            _runFunc = runFunc;
            _checkFunc = checkFunc;
            _completeFunc = completeFunc;
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
                for (nuint i = 0; i < _count; i++)
                    _runFunc(i);
            }
            else
            {
                for (nuint i = 0; i < _count; i++)
                    if (_checkFunc(i))
                        _runFunc(i);
            }

            if (_completeFunc != null)
                _completeFunc();
        }

        public void ScheduleParallel()
        {

        }
    }

    public class SchedulableIter : ISchedulable
    {
        internal delegate InteropBool GetNextIterFn(out nuint outVal);
        public delegate void RunFn(nuint val);

        private IEnumerable<nuint> _iterFunc;
        private Action _completeFunc;
        private Func<nuint, bool> _checkFunc;
        private RunFn _runFunc;

        public SchedulableIter(
            IEnumerable<nuint> iter,
            RunFn runFunc,
            Func<nuint, bool> checkFunc = null,
            Action completeFunc = null
        )
        {
            _iterFunc = iter;
            _runFunc = runFunc;
            _checkFunc = checkFunc;
            _completeFunc = completeFunc;
        }

        public void Run()
        {
            if (_checkFunc == null)
            {
                foreach (nuint val in _iterFunc)
                    _runFunc(val);
            }
            else
            {
                foreach (nuint val in _iterFunc)
                    if (_checkFunc(val))
                        _runFunc(val);
            }

            if (_completeFunc != null)
                _completeFunc();
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

            if (_completeFunc != null)
                _completeFunc();
        }

        [DllImport("__Internal")]
        internal static extern void Native_SchedulableIter_Schedule(GetNextIterFn getNext, RunFn run);
    }
}
