#pragma once

namespace Heart
{
    class HString;
    class PlatformUtils
    {
    public:
        static void InitializePlatform();
        static void ShutdownPlatform();
        static void* LoadDynamicLibrary(const HString& path);
        static void FreeDynamicLibrary(void* lib);
        static void* GetDynamicLibraryExport(void* lib, const HString& name);
        static const char* GetDynamicLibraryExtension();
        static void* GetCurrentModuleHandle();
    };
}