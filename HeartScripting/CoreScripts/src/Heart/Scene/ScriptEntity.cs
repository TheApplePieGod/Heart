using Heart.Container;
using Heart.Core;
using Heart.Math;
using Heart.NativeBridge;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public abstract class ScriptEntity : Entity
    {
        // Client overridable methods
        protected internal virtual void OnConstruct() {}
        protected internal virtual void OnPlayStart() {}
        protected internal virtual void OnPlayEnd() {}
        protected internal virtual void OnUpdate(Timestep timestep) {}
        protected internal virtual void OnCollisionStarted(Entity other) {}
        protected internal virtual void OnCollisionEnded(Entity other) {}
        protected internal virtual void OnScriptFieldChanged(string field, Variant value) {}

        // Generated methods
        public virtual bool GENERATED_SetField(string fieldName, Variant value) { return false; }

        // Having direct delegates for native code to call significantly speeds up performance
        // Not all lifecycle methods are performance-critical, which is why we only have a few
        // in here at the moment
        [UnmanagedCallersOnly]
        internal static void CallOnUpdate(IntPtr entityHandle, double timestep)
        {
            try
            {
                var gcHandle = ManagedGCHandle.FromIntPtr(entityHandle);
                if (gcHandle != null && !gcHandle.IsAlive) return;
                ((ScriptEntity)gcHandle.Target).OnUpdate(new Timestep(timestep));
            }
            catch (Exception e)
            {
                Log.Error("ScriptEntity OnUpdate threw an exception: {0}", e.Message);
            }
        }

        [UnmanagedCallersOnly]
        internal static void CallOnCollisionStarted(IntPtr entityHandle, uint otherHandle, IntPtr sceneHandle)
        {
            try
            {
                var gcHandle = ManagedGCHandle.FromIntPtr(entityHandle);
                if (gcHandle != null && !gcHandle.IsAlive) return;
                ((ScriptEntity)gcHandle.Target).OnCollisionStarted(new Entity(otherHandle, sceneHandle));
            }
            catch (Exception e)
            {
                Log.Error("ScriptEntity OnCollisionStarted threw an exception: {0}", e.Message);
            }
        }

        [UnmanagedCallersOnly]
        internal static void CallOnCollisionEnded(IntPtr entityHandle, uint otherHandle, IntPtr sceneHandle)
        {
            try
            {
                var gcHandle = ManagedGCHandle.FromIntPtr(entityHandle);
                if (gcHandle != null && !gcHandle.IsAlive) return;
                ((ScriptEntity)gcHandle.Target).OnCollisionEnded(new Entity(otherHandle, sceneHandle));
            }
            catch (Exception e)
            {
                Log.Error("ScriptEntity OnCollisionStarted threw an exception: {0}", e.Message);
            }
        }
    }
}
