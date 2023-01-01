#pragma once

namespace Heart
{
    class HStringView8;
    class HString8;
    class PlatformUtils
    {
    public:
        static void InitializePlatform();
        static void ShutdownPlatform();
        static void* LoadDynamicLibrary(const HStringView8& path);
        static void FreeDynamicLibrary(void* lib);
        static void* GetDynamicLibraryExport(void* lib, const HStringView8& name);
        static const char* GetDynamicLibraryExtension();
        static void* GetCurrentModuleHandle();
        static int ExecuteCommand(HStringView8 command);
        static int ExecuteCommandWithOutput(HStringView8 command, HString8& output);
    };
}
