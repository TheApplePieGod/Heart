using System;
using System.Collections.Concurrent;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace Heart.NativeBridge
{
    public class ManagedGCHandle
    {
        private static ConditionalWeakTable<
            AssemblyLoadContext,
            object
        > _unloadingAlcs = new();
        private static ConcurrentDictionary<
            AssemblyLoadContext,
            ConcurrentDictionary<GCHandle, object>
        > _alcStrongHandles = new();

        private GCHandle _handle;

        public static bool IsAlcUnloading(AssemblyLoadContext alc)
            => _unloadingAlcs.TryGetValue(alc, out _);

        public static ManagedGCHandle FromIntPtr(IntPtr ptr)
        {
            ManagedGCHandle newHandle = new ManagedGCHandle();
            newHandle._handle = GCHandle.FromIntPtr(ptr);

            return newHandle;
        }

        public static ManagedGCHandle AllocStrong(object value)
        {
            if (value == null)
                return null;

            ManagedGCHandle newHandle = new ManagedGCHandle();

            var alc = AssemblyLoadContext.GetLoadContext(value.GetType().Assembly);
            if (alc != null)
            {
                // We want a weak handle in case the alc happens to be unloading
                // so that we don't prevent it
                var weakHandle = GCHandle.Alloc(value, GCHandleType.Weak);

                if (!IsAlcUnloading(alc))
                {
                    // Get or create an entry for the current alc
                    var handles = _alcStrongHandles.GetOrAdd(
                        alc,
                        static alc => // Make the lambda static to ensure we don't capture the surrounding value of alc
                        {
                            alc.Unloading += OnAlcUnloading;
                            return new();
                        }
                    );

                    // Store a weak reference along with the object so that we can control
                    // the garbage collection of this object when it comes time for the
                    // alc to unload
                    handles.TryAdd(weakHandle, value);
                }

                newHandle._handle = weakHandle;

                return newHandle;
            }

            newHandle._handle = GCHandle.Alloc(value, GCHandleType.Normal);

            return newHandle;
        }

        public static ManagedGCHandle AllocWeak(object value)
        {
            if (value == null)
                return null;

            ManagedGCHandle newHandle = new ManagedGCHandle();
            newHandle._handle = GCHandle.Alloc(value, GCHandleType.Weak);

            return newHandle;
        }

        public void Free()
        {
            // Remove this handle from the current alc's list of stored
            // handles if applicable
            var target = _handle.Target;
            if (target != null)
            {
                var alc = AssemblyLoadContext.GetLoadContext(target.GetType().Assembly);
                if (alc != null && _alcStrongHandles.TryGetValue(alc, out var handles))
                    handles.TryRemove(_handle, out _);
            }

            _handle.Free();
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void OnAlcUnloading(AssemblyLoadContext alc)
        {
            _unloadingAlcs.Add(alc, null);

            // When we clear out the alc's stored handles (which includes the objects themselves),
            // it will allow gc to occur
            if (_alcStrongHandles.TryRemove(alc, out var handles))
                handles.Clear();
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool IsAlive()
            => _handle.IsAllocated;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public object GetTarget()
            => _handle.Target;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public IntPtr ToIntPtr()
            => GCHandle.ToIntPtr(_handle);
    }
}
