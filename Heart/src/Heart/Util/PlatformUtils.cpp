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
        #else
            void* lib = dlopen(path.Data(), RTLD_LAZY | RTLD_LOCAL);
            return lib;
        #endif

        return nullptr;
    }

    void PlatformUtils::FreeDynamicLibrary(void* lib)
    {
        #ifdef HE_PLATFORM_WINDOWS
            FreeLibrary((HMODULE)lib);
        #else
            dlclose(lib);
        #endif
    }

    void* PlatformUtils::GetDynamicLibraryExport(void* lib, const HStringView8& name)
    {
        #ifdef HE_PLATFORM_WINDOWS
            void* func = GetProcAddress((HMODULE)lib, name.Data());
            return func;
        #else
            void* func = dlsym(lib, name.Data());
            return func;
        #endif

        return nullptr;
    }

    const char* PlatformUtils::GetDynamicLibraryExtension()
    {
        #ifdef HE_PLATFORM_WINDOWS
            return ".dll";
        #elif defined(HE_PLATFORM_LINUX) || defined(HE_PLATFORM_ANDROID)
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
        #else
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
            SECURITY_ATTRIBUTES saAttr; 
            saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
            saAttr.bInheritHandle = TRUE; 
            saAttr.lpSecurityDescriptor = NULL; 

            // Create stdout pipe
            HANDLE hChildStd_OUT_Rd = NULL;
            HANDLE hChildStd_OUT_Wr = NULL;
            if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) 
                return 1;
            if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
                return 1;

            // Create child process 
            PROCESS_INFORMATION piProcInfo; 
            STARTUPINFO siStartInfo;
            BOOL bSuccess = FALSE; 
            
            ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
            ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
            siStartInfo.cb = sizeof(STARTUPINFO); 
            siStartInfo.hStdError = hChildStd_OUT_Wr;
            siStartInfo.hStdOutput = hChildStd_OUT_Wr;
            siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
            
            bSuccess = CreateProcess("C:/Windows/System32/cmd.exe",
                (LPSTR)command.Data(), // command line 
                NULL,                  // process security attributes 
                NULL,                  // primary thread security attributes 
                TRUE,                  // handles are inherited 
                CREATE_NO_WINDOW,      // creation flags 
                NULL,                  // use parent's environment 
                NULL,                  // use parent's current directory 
                &siStartInfo,          // STARTUPINFO pointer 
                &piProcInfo            // receives PROCESS_INFORMATION
            ); 
            
            if (!bSuccess) 
                return 1;
            else 
            {
                CloseHandle(piProcInfo.hThread);
                CloseHandle(hChildStd_OUT_Wr);
            }
            
            // Read output from pipe
            DWORD dwRead; 
            DWORD bytesAvail;
            CHAR* chBuf = new char[4096]; 
            bSuccess = FALSE;
            while (true) 
            { 
                bSuccess = ReadFile(hChildStd_OUT_Rd, chBuf, 4096, &dwRead, NULL);
                if(!bSuccess || dwRead == 0) break; 

                output += HStringView8(chBuf, dwRead);
            }

            DWORD result;
            GetExitCodeProcess(piProcInfo.hProcess, &result);

            // Cleanup
            CloseHandle(piProcInfo.hProcess);
            CloseHandle(hChildStd_OUT_Rd);
            delete[] chBuf;

            return (int)result;
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
