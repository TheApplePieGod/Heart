#include "hepch.h"
#include "PlatformUtils.h"

namespace Heart
{
    void* PlatformUtils::LoadDynamicLibrary(const std::string& path)
    {
        #ifdef HE_PLATFORM_WINDOWS
            HMODULE lib = LoadLibraryA(path.c_str());
            return (void*)lib;
        #elif defined(HE_PLATFORM_LINUX) || defined(HE_PLATFORM_MACOS)
            void* lib = dlopen(lib, RTLD_LAZY, RTLD_LOCAL);
            return lib;
        #endif

        return nullptr;
    }

    void PlatformUtils::FreeDynamicLibrary(void* lib)
    {
        #ifdef HE_PLATFORM_WINDOWS
            FreeLibrary((HMODULE)lib);
        #elif defined(HE_PLATFORM_LINUX) || defined(HE_PLATFORM_MACOS)
            dlclose(lib);
        #endif
    }

    void* PlatformUtils::GetDynamicLibraryExport(void* lib, const std::string& name)
    {
        #ifdef HE_PLATFORM_WINDOWS
            void* func = GetProcAddress((HMODULE)lib, name.c_str());
            return func;
        #elif defined(HE_PLATFORM_LINUX) || defined(HE_PLATFORM_MACOS)
            void* func = dlsym(lib, name.c_str());
            return func;
        #endif

        return nullptr;
    }

    const char* PlatformUtils::GetDynamicLibraryExtension()
    {
        #ifdef HE_PLATFORM_WINDOWS
            return ".dll";
        #elif defined(HE_PLATFORM_LINUX)
            return ".so";
        #elif defined(HE_PLATFORM_MACOS)
            return ".dylib";
        #endif

        return "";
    }

    void* PlatformUtils::GetCurrentModuleHandle()
    {
        #ifdef HE_PLATFORM_WINDOWS
            return GetModuleHandleA(0);
        #elif defined(HE_PLATFORM_LINUX) || defined(HE_PLATFORM_MACOS)
            return dlopen(nullptr, RTLD_NOW);
        #endif

        return nullptr;
    }

    std::string PlatformUtils::WideToNarrowString(const std::wstring& wide)
    {
        std::string output;
        output.reserve(wide.length());

        for (wchar c : wide)
            output.push_back((char)c);

        return output;
    }

    std::wstring PlatformUtils::NarrowToWideString(const std::string& narrow)
    {
        std::wstring output;
        output.reserve(narrow.length());

        for (char c : narrow)
            output.push_back((wchar)c);

        return output;
    }

    void PlatformUtils::InitializePlatform()
    {
        #ifdef HE_PLATFORM_WINDOWS
            CoInitialize(NULL);
        #endif
    }

    void PlatformUtils::ShutdownPlatform()
    {
        #ifdef HE_PLATFORM_WINDOWS
            CoUninitialize();
        #endif
    }
}