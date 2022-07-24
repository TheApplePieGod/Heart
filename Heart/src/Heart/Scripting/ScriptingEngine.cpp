#include "hepch.h"
#include "ScriptingEngine.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Util/PlatformUtils.h"
#include "hostfxr.h"
#include "coreclr_delegates.h"

// Link to error codes that can be returned by hostfxr functions
// will need to convert int result into hex
// https://stackoverflow.com/questions/37205883/where-i-could-find-a-reference-for-all-the-cor-e-hresults-wrapped-by-net-fra
// https://referencesource.microsoft.com/#mscorlib/system/__hresults.cs

// We need this in order to ensure that the dllexports inside the engine static lib
// do not get removed
extern void* heartInteropFunctions[100];
[[maybe_unused]] volatile void** exportHeartInteropFunctions;

namespace Heart
{
    hostfxr_initialize_for_runtime_config_fn s_ConfigInitFunc;
    hostfxr_get_runtime_delegate_fn s_GetDelegateFunc;
    hostfxr_close_fn s_CloseFunc;
    void* s_HostFXRHandle;

    using HostFXRString = std::basic_string<char_t>;
    using EntryPointInitializeFn = int (*)(void*);

    HostFXRString StringToHostFXR(const std::string& str)
    {
        #ifdef HE_PLATFORM_WINDOWS
            return PlatformUtils::NarrowToWideString(str);
        #else
            return str;
        #endif
    }

    #ifdef HE_PLATFORM_WINDOWS
        #define HOSTFXR_STR(str) L##str
    #else
        #define HOSTFXR_STR(str) str
    #endif

    bool LoadHostFXR()
    {
        char hostfxrName[100];
        sprintf(hostfxrName, "hostfxr%s", PlatformUtils::GetDynamicLibraryExtension());

        s_HostFXRHandle = PlatformUtils::LoadDynamicLibrary(hostfxrName);
        if (!s_HostFXRHandle) return false; 

        s_ConfigInitFunc = (hostfxr_initialize_for_runtime_config_fn)
            PlatformUtils::GetDynamicLibraryExport(
                s_HostFXRHandle,
                "hostfxr_initialize_for_runtime_config"
            );
        s_GetDelegateFunc = (hostfxr_get_runtime_delegate_fn)
            PlatformUtils::GetDynamicLibraryExport(
                s_HostFXRHandle,
                "hostfxr_get_runtime_delegate"
            );
        s_CloseFunc = (hostfxr_close_fn)
            PlatformUtils::GetDynamicLibraryExport(
                s_HostFXRHandle,
                "hostfxr_close"
            );

        return (s_ConfigInitFunc && s_GetDelegateFunc && s_CloseFunc);
    }

    load_assembly_and_get_function_pointer_fn InitHostFXRWithConfig(const char_t* configPath)
    {
        hostfxr_handle ctx = nullptr;
        int rc = s_ConfigInitFunc(configPath, nullptr, &ctx);
        if (rc != 0 || !ctx)
        {
            s_CloseFunc(ctx);
            return nullptr;
        }

        void* loadAssemblyFunc = nullptr;
        rc = s_GetDelegateFunc(ctx, hdt_load_assembly_and_get_function_pointer, &loadAssemblyFunc);

        s_CloseFunc(ctx);

        return (load_assembly_and_get_function_pointer_fn)loadAssemblyFunc;
    }

    void ScriptingEngine::Initialize()
    {
        exportHeartInteropFunctions = (volatile void**)heartInteropFunctions;

        bool result = LoadHostFXR();
        HE_ENGINE_ASSERT(result, "Failed to load hostfxr");

        load_assembly_and_get_function_pointer_fn loadAssemblyWithPtrFunc = 
            InitHostFXRWithConfig(HOSTFXR_STR("scripting/CoreScripts.runtimeconfig.json"));
        HE_ENGINE_ASSERT(loadAssemblyWithPtrFunc, "Failed to initialize hostfxr with config");
        HE_ENGINE_LOG_DEBUG(".NET hostfxr initialized");

        EntryPointInitializeFn mainFunc = nullptr;
        int rc = loadAssemblyWithPtrFunc(
            HOSTFXR_STR("scripting/CoreScripts.dll"),
            HOSTFXR_STR("Heart.EntryPoint, CoreScripts"),
            HOSTFXR_STR("Initialize"),
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            (void**)&mainFunc
        );
        HE_ENGINE_ASSERT(rc == 0, "Failed to load scripting entrypoint func");

        int res = mainFunc(PlatformUtils::GetCurrentModuleHandle());
        int d = 0;
    }

    void ScriptingEngine::Shutdown()
    {
        if (s_HostFXRHandle)
        {
            PlatformUtils::FreeDynamicLibrary(s_HostFXRHandle);
            s_HostFXRHandle = nullptr;
        }
    }
}