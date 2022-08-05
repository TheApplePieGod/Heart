#include "hepch.h"
#include "PlatformUtils.h"

#include "Heart/Container/HString.h"

namespace Heart
{
    void* PlatformUtils::LoadDynamicLibrary(const HStringView& path)
    {
        #ifdef HE_PLATFORM_WINDOWS
            HMODULE lib = LoadLibraryA(path.DataUTF8());
            return (void*)lib;
        #elif defined(HE_PLATFORM_LINUX) || defined(HE_PLATFORM_MACOS)
            void* lib = dlopen(path.DataUTF8(), RTLD_LAZY, RTLD_LOCAL);
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

    void* PlatformUtils::GetDynamicLibraryExport(void* lib, const HStringView& name)
    {
        #ifdef HE_PLATFORM_WINDOWS
            void* func = GetProcAddress((HMODULE)lib, name.DataUTF8());
            return func;
        #elif defined(HE_PLATFORM_LINUX) || defined(HE_PLATFORM_MACOS)
            void* func = dlsym(lib, name.DataUTF8());
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