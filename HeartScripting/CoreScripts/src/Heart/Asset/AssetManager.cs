using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.NativeInterop;
using Heart.Scene;

namespace Heart.Asset
{
    public class AssetManager
    {
        public static UUID GetAssetUUID(string path, bool isResource = false)
        {
            Native_AssetManager_GetAssetUUID(path, NativeMarshal.BoolToInteropBool(isResource), out var uuid);
            return uuid;
        }

        [DllImport("__Internal")]
        internal static extern void Native_AssetManager_GetAssetUUID([MarshalAs(UnmanagedType.LPStr)] string path, InteropBool isResource, out UUID outId);
    }
}
