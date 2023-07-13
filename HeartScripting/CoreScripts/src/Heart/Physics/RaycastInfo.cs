using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.NativeInterop;
using Heart.Math;

namespace Heart.Physics
{
    [StructLayout(LayoutKind.Explicit, Size = 33)]
    internal struct RaycastInfoInternal
    {
        [FieldOffset(0)] public uint TraceChannels;
        [FieldOffset(4)] public uint TraceMask;
        [FieldOffset(8)] public Vec3Internal Start;
        [FieldOffset(20)] public Vec3Internal End;
        [FieldOffset(32)] public InteropBool DrawDebugLine;
    }
    
    public class RaycastInfo
    {
        internal RaycastInfoInternal _internal;

        public RaycastInfo()
        {
            _internal.TraceChannels = (uint)DefaultCollisionChannel.Default;
            _internal.TraceMask = (uint)DefaultCollisionChannel.All;
            _internal.Start = new Vec3Internal { X = 0.0f, Y = 0.0f, Z = 0.0f };
            _internal.End = new Vec3Internal { X = 0.0f, Y = 0.0f, Z = 0.0f };
            _internal.DrawDebugLine = InteropBool.False;
        }

        public RaycastInfo(uint traceChannels, uint traceMask, Vec3 start, Vec3 end, bool drawDebugLine = false) 
        {
            _internal.TraceChannels = traceChannels;
            _internal.TraceMask = traceMask;
            _internal.Start = start._internal;
            _internal.End = end._internal;
            _internal.DrawDebugLine = NativeMarshal.BoolToInteropBool(drawDebugLine);
        }

        public RaycastInfo(RaycastInfo other)
        {
            _internal = other._internal;
        }

        internal RaycastInfo(RaycastInfoInternal other)
        {
            _internal = other;
        }

        public uint TraceChannels
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.TraceChannels;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.TraceChannels = value;
        }

        public uint TraceMask
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.TraceMask;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.TraceMask = value;
        }
        
        public Vec3 Start
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec3(_internal.Start);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.Start = value._internal;
        }

        public Vec3 End
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec3(_internal.End);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.End = value._internal;
        }

        public bool DrawDebugLine
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => NativeMarshal.InteropBoolToBool(_internal.DrawDebugLine);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.DrawDebugLine = NativeMarshal.BoolToInteropBool(value);
        }
    }
}