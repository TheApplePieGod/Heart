﻿using Heart.Container;
using Heart.NativeInterop;
using Heart.Scene;
using System;

namespace Heart.NativeBridge
{
    internal unsafe struct ManagedCallbacks
    {
        public delegate* unmanaged<HArrayInternal*, void> ClientReflection_GetClientInstantiableClasses;
        public delegate* unmanaged<HArrayInternal*, void> ClientReflection_GetClientComponentClasses;
        public delegate* unmanaged<HStringInternal*, HArrayInternal*, void> ClientReflection_GetClientSerializableFields;
        public delegate* unmanaged<HStringInternal*, uint, IntPtr, IntPtr> ManagedObject_InstantiateClientScriptEntity;
        public delegate* unmanaged<IntPtr, void> ManagedObject_DestroyObject;
        public delegate* unmanaged<IntPtr, HStringInternal*, HArrayInternal*, InteropBool> ManagedObject_InvokeFunction;
        public delegate* unmanaged<IntPtr, HStringInternal*, Variant*, void> ManagedObject_GetFieldValue;
        public delegate* unmanaged<IntPtr, HStringInternal*, Variant, InteropBool, InteropBool> ManagedObject_SetFieldValue;
        public delegate* unmanaged<IntPtr, double, void> ScriptEntity_CallOnUpdate;
        public delegate* unmanaged<IntPtr, uint, IntPtr, void> ScriptEntity_CallOnCollisionStarted;
        public delegate* unmanaged<IntPtr, uint, IntPtr, void> ScriptEntity_CallOnCollisionEnded;

        public static void Get(IntPtr outCallbacks)
        {
            ManagedCallbacks* outVal = (ManagedCallbacks*)outCallbacks;
            *outVal = new() {
                ClientReflection_GetClientInstantiableClasses = &ClientReflection.GetClientInstantiableClasses,
                ClientReflection_GetClientComponentClasses = &ClientReflection.GetClientComponentClasses,
                ClientReflection_GetClientSerializableFields = &ClientReflection.GetClientSerializableFields,
                ManagedObject_InstantiateClientScriptEntity = &ManagedObject.InstantiateClientScriptEntity,
                ManagedObject_DestroyObject = &ManagedObject.DestroyObject,
                ManagedObject_InvokeFunction = &ManagedObject.InvokeFunction,
                ManagedObject_GetFieldValue = &ManagedObject.GetFieldValue,
                ManagedObject_SetFieldValue = &ManagedObject.SetFieldValue,
                ScriptEntity_CallOnUpdate = &ScriptEntity.CallOnUpdate,
                ScriptEntity_CallOnCollisionStarted = &ScriptEntity.CallOnCollisionStarted,
                ScriptEntity_CallOnCollisionEnded = &ScriptEntity.CallOnCollisionEnded
            };
        }
    }
}
