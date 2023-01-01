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

    int PlatformUtils::ExecuteCommand(HStringView8 command)
    {
        return system(command.Data());
    }

    // https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
    // https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output?redirectedfrom=MSDN
    int PlatformUtils::ExecuteCommandWithOutput(HStringView8 command, HString8& output)
    {
        #ifdef HE_PLATFORM_WINDOWS
            HE_ENGINE_ASSERT(false, "Not implemented");
            return 1;
        #else
            std::array<char, 128> buffer;
            std::string result;
            FILE* pipe = popen(command.Data(), "r");
            if (!pipe) {
                throw std::runtime_error("popen() failed!");
            }
            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                result += buffer.data();
            }
            output = result;
            return pclose(pipe);
        #endif
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
