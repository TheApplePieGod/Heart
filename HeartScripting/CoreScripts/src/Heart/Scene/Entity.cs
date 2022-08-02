using Heart.Container;
using Heart.Core;
using Heart.NativeBridge;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public abstract class Entity
    {
        // Client overridable methods
        public virtual void OnPlayStart() {}
        public virtual void OnPlayEnd() {}
        public virtual void OnUpdate(Timestep timestep) {}

        // Client callable methods
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T GetComponent<T>() where T : Component
        {
            return ComponentUtils.GetComponent<T>(_entityHandle, _sceneHandle);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool HasComponent<T>() where T : Component
        {
            return ComponentUtils.HasComponent<T>(_entityHandle, _sceneHandle);
        }

        // Internal fields
        internal uint _entityHandle = uint.MaxValue;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        // Generated methods
        public virtual bool GENERATED_SetField(string fieldName, Variant value) { return false; }

        // Having direct delegates for native code to call significantly speeds up performance
        // Not all lifecycle methods are performance-critical, which is why we only have OnUpdate
        // in here at the moment
        [UnmanagedCallersOnly]
        internal static void CallOnUpdate(IntPtr entityHandle, double timestep)
        {
            var gcHandle = ManagedGCHandle.FromIntPtr(entityHandle);
            if (gcHandle != null && !gcHandle.IsAlive) return;

            try
            {
                ((Entity)gcHandle.Target).OnUpdate(new Timestep(timestep));
            }
            catch (Exception e)
            {
                Log.Error("Entity OnUpdate threw an exception: {0}", e.Message);
            }
        }
    }
}
