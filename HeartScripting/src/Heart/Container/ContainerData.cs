using System.Runtime.InteropServices;

namespace Heart.Container
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ContainerInfo
    {
        public uint RefCount;
        public uint AllocatedCount;
        public uint ElemCount;
    }
}
