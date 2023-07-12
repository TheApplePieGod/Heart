using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.NativeInterop;

namespace Heart.Scene
{
    public class EntityView : IEnumerable<Entity>, IDisposable
    {
        internal IntPtr _internalVal;
        private Scene _scene;

        public EntityView(Scene scene)
        {
            _scene = scene;
            Native_EntityView_Init(out _internalVal, _scene._internalValue);
        }

        ~EntityView()
        {
            Native_EntityView_Destroy(_internalVal);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            Native_EntityView_Destroy(_internalVal);
        }

        public IEnumerator<Entity> GetEnumerator()
        {
            while (NativeMarshal.InteropBoolToBool(Native_EntityView_GetNext(_internalVal, out uint entityHandle)))
                yield return new Entity(entityHandle, _scene._internalValue);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        [DllImport("__Internal")]
        internal static extern void Native_EntityView_Init(out IntPtr view, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_EntityView_Destroy(IntPtr view);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_EntityView_GetNext(IntPtr view, out uint entityHandle);
    }
}
