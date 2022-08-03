#pragma once

namespace Heart
{
    class PlatformUtils
    {
    public:
        static void InitializePlatform();
        static void ShutdownPlatform();
        static void* LoadDynamicLibrary(const std::string& path);
        static void FreeDynamicLibrary(void* lib);
        static void* GetDynamicLibraryExport(void* lib, const std::string& name);
        static const char* GetDynamicLibraryExtension();
        static void* GetCurrentModuleHandle();
        static std::string WideToNarrowString(const std::wstring& wide);
        static std::wstring NarrowToWideString(const std::string& narrow);
    };
}