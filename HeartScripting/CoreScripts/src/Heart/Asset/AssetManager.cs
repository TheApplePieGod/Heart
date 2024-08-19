using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.NativeInterop;
using Heart.NativeBridge;
using Heart.Scene;

namespace Heart.Asset
{
    public partial class AssetManager
    {
        public static unsafe UUID GetAssetUUID(string path, bool isResource = false)
        {
            UUID uuid = 0;

            fixed (char* ptr = path)
            {
                Native_AssetManager_GetAssetUUID(ptr, (uint)path.Length, NativeMarshal.BoolToInteropBool(isResource), out uuid);
            }

            return uuid;
        }

        [UnmanagedCallback]
        internal static unsafe partial void Native_AssetManager_GetAssetUUID(char* path, uint pathLen, InteropBool isResource, out UUID outId);
    }
}
