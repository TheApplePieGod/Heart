#include "hepch.h"
#include "PlatformUtils.h"

#include "Heart/Container/HString8.h"

#ifdef HE_PLATFORM_MACOS
#include "Heart/Platform/MacOS/Utils.h"
#endif

namespace Heart
{
    void* PlatformUtils::LoadDynamicLibrary(const HStringView8& path)
    {
        #ifdef HE_PLATFORM_WINDOWS
            HMODULE lib = LoadLibraryA(path.Data());
            return (void*)lib;
        #elif defined(HE_PLATFORM_LINUX) || defined(HE_PLATFORM_MACOS)
            void* lib = dlopen(path.Data(), RTLD_LAZY | RTLD_LOCAL);
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

    void* PlatformUtils::GetDynamicLibraryExport(void* lib, const HStringView8& name)
    {
        #ifdef HE_PLATFORM_WINDOWS
            void* func = GetProcAddress((HMODULE)lib, name.Data());
            return func;
        #elif defined(HE_PLATFORM_LINUX) || defined(HE_PLATFORM_MACOS)
            void* func = dlsym(lib, name.Data());
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
        #elif defined(HE_PLATFORM_MACOS)
            MacOS::Utils::SetCorrectWorkingDirectory();
        #endif
    }

    void PlatformUtils::ShutdownPlatform()
    {
        #ifdef HE_PLATFORM_WINDOWS
            CoUninitialize();
        #endif
    }
}