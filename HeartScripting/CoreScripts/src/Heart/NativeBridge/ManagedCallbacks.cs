using Heart.Container;
using Heart.NativeInterop;
using Heart.Plugins;
using Heart.Scene;
using System;

namespace Heart.NativeBridge
{
    internal unsafe struct ManagedCallbacks
    {
        public delegate* unmanaged<IntPtr, HArrayInternal*, InteropBool> EntryPoint_LoadClientPlugin;
        public delegate* unmanaged<InteropBool> EntryPoint_UnloadClientPlugin;
        public delegate* unmanaged<HStringInternal*, HArrayInternal*, void> PluginReflection_GetClientSerializableFields;
        public delegate* unmanaged<HStringInternal*, uint, IntPtr, IntPtr> ManagedObject_InstantiateClientEntity;
        public delegate* unmanaged<IntPtr, void> ManagedObject_DestroyObject;
        public delegate* unmanaged<IntPtr, HStringInternal*, HArrayInternal*, InteropBool> ManagedObject_InvokeFunction;
        public delegate* unmanaged<IntPtr, HStringInternal*, Variant*, void> ManagedObject_GetFieldValue;
        public delegate* unmanaged<IntPtr, HStringInternal*, Variant, InteropBool> ManagedObject_SetFieldValue;
        public delegate* unmanaged<IntPtr, double, void> Entity_CallOnUpdate;

        public static ManagedCallbacks Get()
        {
            return new()
            {
                EntryPoint_LoadClientPlugin = &EntryPoint.LoadClientPlugin,
                EntryPoint_UnloadClientPlugin = &EntryPoint.UnloadClientPlugin,
                PluginReflection_GetClientSerializableFields = &PluginReflection.GetClientSerializableFields,
                ManagedObject_InstantiateClientEntity = &ManagedObject.InstantiateClientEntity,
                ManagedObject_DestroyObject = &ManagedObject.DestroyObject,
                ManagedObject_InvokeFunction = &ManagedObject.InvokeFunction,
                ManagedObject_GetFieldValue = &ManagedObject.GetFieldValue,
                ManagedObject_SetFieldValue = &ManagedObject.SetFieldValue,
                Entity_CallOnUpdate = &Entity.CallOnUpdate,
            };
        }
    }
}
