#include "hepch.h"
#include "ScriptingEngine.h"

#include "Heart/Core/Timing.h"
#include "Heart/Core/Timestep.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Container/HArray.h"
#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Util/PlatformUtils.h"
#include "hostfxr.h"
#include "coreclr_delegates.h"

// Link to error codes that can be returned by hostfxr functions
// will need to convert int result into hex
// https://stackoverflow.com/questions/37205883/where-i-could-find-a-reference-for-all-the-cor-e-hresults-wrapped-by-net-fra
// https://referencesource.microsoft.com/#mscorlib/system/__hresults.cs

// See NativeCallbacks.cpp for more info
extern void* exportVariable;
[[maybe_unused]] volatile void* exportVariableSet;

namespace Heart
{
    hostfxr_initialize_for_dotnet_command_line_fn s_CmdInitFunc;
    hostfxr_initialize_for_runtime_config_fn s_ConfigInitFunc;
    hostfxr_get_runtime_delegate_fn s_GetDelegateFunc;
    hostfxr_close_fn s_CloseFunc;
    void* s_HostFXRHandle;

    using InitializeFn = bool (*)(void*, ManagedCallbacks*);

    #ifdef HE_PLATFORM_WINDOWS
        #define HOSTFXR_STR(str) L##str
    #else
        #define HOSTFXR_STR(str) str
    #endif

    #define UTF16_STR(str) u##str

    bool LoadHostFXR()
    {
        #ifdef HE_PLATFORM_WINDOWS
        const char* hostfxrName = "scripting/hostfxr.dll";
        #elif defined(HE_PLATFORM_MACOS)
        const char* hostfxrName = "scripting/libhostfxr.dylib";
        #else
        const char* hostfxrName = "scripting/hostfxr.so";
        #endif

        s_HostFXRHandle = PlatformUtils::LoadDynamicLibrary(hostfxrName);
        if (!s_HostFXRHandle) return false; 

        s_CmdInitFunc = (hostfxr_initialize_for_dotnet_command_line_fn)
            PlatformUtils::GetDynamicLibraryExport(
                s_HostFXRHandle,
                "hostfxr_initialize_for_dotnet_command_line"
            );
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

        return (s_CmdInitFunc && s_ConfigInitFunc && s_GetDelegateFunc && s_CloseFunc);
    }

    void InitHostFXRForCmdline(const char_t* assemblyPath, load_assembly_and_get_function_pointer_fn& outLoadAssemblyFunc)
    {
        hostfxr_handle ctx = nullptr;
        int rc = s_CmdInitFunc(1, &assemblyPath, nullptr, &ctx);
        if (rc != 0 || !ctx)
        {
            s_CloseFunc(ctx);
            return;
        }

        void* loadAssemblyFunc = nullptr;
        rc = s_GetDelegateFunc(ctx, hdt_load_assembly_and_get_function_pointer, &loadAssemblyFunc);

        s_CloseFunc(ctx);

        outLoadAssemblyFunc = (load_assembly_and_get_function_pointer_fn)loadAssemblyFunc;
    }

    void InitHostFXRWithConfig(const char_t* configPath, load_assembly_and_get_function_pointer_fn& outLoadAssemblyFunc)
    {
        // TODO: this is mid, but we don't need to mess around with this on windows and allegedly
        // the macos dotnet is always installed in the same location
        hostfxr_initialize_parameters params{};
        #ifdef HE_PLATFORM_MACOS
        params.dotnet_root = "/usr/local/share/dotnet";
        #endif

        hostfxr_handle ctx = nullptr;
        int rc = s_ConfigInitFunc(configPath, &params, &ctx);
        if (rc != 0 || !ctx)
        {
            s_CloseFunc(ctx);
            return;
        }

        void* loadAssemblyFunc = nullptr;
        rc = s_GetDelegateFunc(ctx, hdt_load_assembly_and_get_function_pointer, &loadAssemblyFunc);

        s_CloseFunc(ctx);

        outLoadAssemblyFunc = (load_assembly_and_get_function_pointer_fn)loadAssemblyFunc;
    }

    void ScriptingEngine::Initialize()
    {
        exportVariableSet = exportVariable;

        bool result = LoadHostFXR();
        HE_ENGINE_ASSERT(result, "Failed to load hostfxr");

        load_assembly_and_get_function_pointer_fn loadAssemblyWithPtrFunc;
        #ifdef HE_DIST
            InitHostFXRForCmdline(HOSTFXR_STR("scripting/CoreScripts.dll"), loadAssemblyWithPtrFunc);
        #else
            InitHostFXRWithConfig(HOSTFXR_STR("scripting/CoreScripts.runtimeconfig.json"), loadAssemblyWithPtrFunc);
        #endif
        HE_ENGINE_ASSERT(loadAssemblyWithPtrFunc, "Failed to initialize hostfxr with config");
        HE_ENGINE_LOG_DEBUG(".NET hostfxr initialized");

        InitializeFn initFunc = nullptr;
        int rc = loadAssemblyWithPtrFunc(
            HOSTFXR_STR("scripting/CoreScripts.dll"),
            HOSTFXR_STR("Heart.EntryPoint, CoreScripts"),
            HOSTFXR_STR("Initialize"),
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            (void**)&initFunc
        );
        HE_ENGINE_ASSERT(rc == 0, "Failed to load scripting entrypoint func");

        bool res = initFunc(PlatformUtils::GetCurrentModuleHandle(), &s_CoreCallbacks);
        HE_ENGINE_ASSERT(res, "Failed to initialize core scripts");

        HE_ENGINE_LOG_DEBUG("Scripts ready");
    }

    void ScriptingEngine::Shutdown()
    {
        if (s_HostFXRHandle)
        {
            PlatformUtils::FreeDynamicLibrary(s_HostFXRHandle);
            s_HostFXRHandle = nullptr;
        }
    }

    bool ScriptingEngine::LoadClientPlugin(const HStringView8& absolutePath)
    {
        auto timer = Timer("Client plugin load"); 

        if (s_ClientPluginLoaded)
        {
            bool res = UnloadClientPlugin();
            if (!res) return false;
        }

        HArray outClasses;
        bool res = s_CoreCallbacks.EntryPoint_LoadClientPlugin(absolutePath.Data(), &outClasses);
        if (res)
        {
            // Populate local array
            for (u32 i = 0; i < outClasses.Count(); i++)
            {
                auto convertedString = outClasses[i].String().Convert(HString::Encoding::UTF8);
                s_InstantiableClasses[convertedString] = ScriptClass(convertedString);
            }

            HE_ENGINE_LOG_INFO("Client plugin successfully loaded");
            s_ClientPluginLoaded = true;
            return true;
        }

        HE_ENGINE_LOG_ERROR("Failed to load client plugin");
        return false;
    }

    bool ScriptingEngine::UnloadClientPlugin()
    {
        if (!s_ClientPluginLoaded) return true;

        s_InstantiableClasses.clear();

        bool res = s_CoreCallbacks.EntryPoint_UnloadClientPlugin();
        if (res)
        {
            HE_ENGINE_LOG_INFO("Client plugin successfully unloaded");
            s_ClientPluginLoaded = false;
            return true;
        }

        HE_ENGINE_LOG_ERROR("Failed to unload client plugin");
        return false;
    }

    uptr ScriptingEngine::InstantiateObject(const HString& type, u32 entityHandle, Scene* sceneHandle)
    {
        HE_PROFILE_FUNCTION();

        uptr result = (uptr)s_CoreCallbacks.ManagedObject_InstantiateClientScriptEntity(&type, entityHandle, sceneHandle);
        if (result == 0)
            HE_ENGINE_LOG_ERROR("Failed to instantiate class '{0}'", type.ToUTF8().Data());
        
        return result;
    }

    void ScriptingEngine::DestroyObject(uptr handle)
    {
        HE_PROFILE_FUNCTION();
        
        s_CoreCallbacks.ManagedObject_DestroyObject(handle);
    }

    bool ScriptingEngine::InvokeFunction(uptr object, const HString& funcName, const HArray& args)
    {
        HE_PROFILE_FUNCTION();

        return s_CoreCallbacks.ManagedObject_InvokeFunction(object, &funcName, &args);
    }

    void ScriptingEngine::InvokeEntityOnUpdate(uptr entity, Timestep timestep)
    {
        HE_PROFILE_FUNCTION();

        s_CoreCallbacks.ScriptEntity_CallOnUpdate(entity, timestep.StepMilliseconds());
    }

    void ScriptingEngine::InvokeEntityOnCollisionStarted(uptr entity, Entity other)
    {
        HE_PROFILE_FUNCTION();

        s_CoreCallbacks.ScriptEntity_CallOnCollisionStarted(entity, (u32)other.GetHandle(), other.GetScene());
    }

    void ScriptingEngine::InvokeEntityOnCollisionEnded(uptr entity, Entity other)
    {
        HE_PROFILE_FUNCTION();

        s_CoreCallbacks.ScriptEntity_CallOnCollisionEnded(entity, (u32)other.GetHandle(), other.GetScene());
    }

    Variant ScriptingEngine::GetFieldValue(uptr entity, const HString& fieldName)
    {
        HE_PROFILE_FUNCTION();

        Variant variant;
        s_CoreCallbacks.ManagedObject_GetFieldValue(entity, &fieldName, &variant);
        return variant;
    }

    bool ScriptingEngine::SetFieldValue(uptr entity, const HString& fieldName, const Variant& value, bool invokeCallback)
    {
        HE_PROFILE_FUNCTION();

        return s_CoreCallbacks.ManagedObject_SetFieldValue(entity, &fieldName, value, invokeCallback);
    }
}
