#pragma once

namespace Heart
{
    class HString;
    class HStringView;
    class PlatformUtils
    {
    public:
        static void InitializePlatform();
        static void ShutdownPlatform();
        static void* LoadDynamicLibrary(const HStringView& path);
        static void FreeDynamicLibrary(void* lib);
        static void* GetDynamicLibraryExport(void* lib, const HStringView& name);
        static const char* GetDynamicLibraryExtension();
        static void* GetCurrentModuleHandle();
    };
}