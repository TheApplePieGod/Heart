using Heart.Container;
using Heart.Core;
using Heart.NativeBridge;
using System;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    [Serializable]
    public abstract class Entity
    {
        public virtual void OnPlayStart() {}

        public virtual void OnPlayEnd() {}

        public virtual void OnUpdate(Timestep timestep) {}

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

            ((Entity)gcHandle.Target).OnUpdate(new Timestep(timestep));
        }
    }
}
