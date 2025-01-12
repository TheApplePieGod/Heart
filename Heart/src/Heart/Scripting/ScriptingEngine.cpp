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

#ifdef HE_PLATFORM_ANDROID
#include "Heart/Platform/Android/AndroidApp.h"
#endif

// Link to error codes that can be returned by hostfxr functions
// will need to convert int result into hex
// https://stackoverflow.com/questions/37205883/where-i-could-find-a-reference-for-all-the-cor-e-hresults-wrapped-by-net-fra
// https://referencesource.microsoft.com/#mscorlib/system/__hresults.cs

extern void* NativeCallbacks[];

namespace Heart
{
    hostfxr_initialize_for_dotnet_command_line_fn s_CmdInitFunc = nullptr;
    hostfxr_initialize_for_runtime_config_fn s_ConfigInitFunc = nullptr;
    hostfxr_get_runtime_delegate_fn s_GetDelegateFunc = nullptr;
    hostfxr_get_dotnet_environment_info_fn s_GetEnvInfoFunc = nullptr;
    hostfxr_close_fn s_CloseFunc = nullptr;
    void* s_HostFXRHandle = nullptr;

    using InitializeFn = bool (*)(void*, BridgeManagedCallbacks*);

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
        #elif defined(HE_PLATFORM_LINUX)
        const char* hostfxrName = "scripting/libhostfxr.so";
        #endif

        #ifdef HE_PLATFORM_ANDROID
        // Check to see if the files exist in local storage or not. If not, we need
        // to copy them over from assets so we can properly load them.
        HString8 basePath(AndroidApp::App->activity->internalDataPath);
        basePath += "/";
        HString8 scriptingPath("scripting/");
        HString8 hostfxrPath = basePath + "libhostfxr.so";
        const char* hostfxrName = hostfxrPath.Data();

        HE_ENGINE_LOG_DEBUG("Internal data path: {0}", basePath.Data());

        // For now, copy files on startup. This is effectively instant so it is OK, but ideally
        // we will want a smarter way to determine when a new version needs to be copied.
        if (true || !std::filesystem::exists(hostfxrName))
        {
            HE_ENGINE_LOG_DEBUG("Copying scripting files");

            AAssetDir* assetDir = AAssetManager_openDir(
                AndroidApp::App->activity->assetManager,
                scriptingPath.Data()
            );
            const char* filename = (const char*)NULL;
            while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) {
                HE_ENGINE_LOG_TRACE("Copying '{0}' to internal storage", filename);
                AAsset* asset = AAssetManager_open(
                    AndroidApp::App->activity->assetManager,
                    (scriptingPath + filename).Data(),
                    AASSET_MODE_STREAMING
                );
                char buf[BUFSIZ];
                int nb_read = 0;
                FILE* out = fopen((basePath + filename).Data(), "w");
                while ((nb_read = AAsset_read(asset, buf, BUFSIZ)) > 0)
                    fwrite(buf, nb_read, 1, out);
                fclose(out);
                AAsset_close(asset);
            }
            AAssetDir_close(assetDir);
        }
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
        s_GetEnvInfoFunc = (hostfxr_get_dotnet_environment_info_fn)
            PlatformUtils::GetDynamicLibraryExport(
                s_HostFXRHandle,
                "hostfxr_get_dotnet_environment_info"
            );
        s_CloseFunc = (hostfxr_close_fn)
            PlatformUtils::GetDynamicLibraryExport(
                s_HostFXRHandle,
                "hostfxr_close"
            );

        return s_CmdInitFunc && s_ConfigInitFunc && s_GetDelegateFunc && s_GetEnvInfoFunc && s_CloseFunc;
    }

    void InitHostFXRForCmdline(const char_t* assemblyPath, load_assembly_and_get_function_pointer_fn& outLoadAssemblyFunc)
    {
        hostfxr_initialize_parameters params{};
        params.size = sizeof(hostfxr_initialize_parameters);
        #ifdef HE_PLATFORM_ANDROID
        // Load assemblies from the location we copied scripting files to
        params.dotnet_root = AndroidApp::App->activity->internalDataPath;
        #endif

        hostfxr_handle ctx = nullptr;
        int rc = s_CmdInitFunc(1, &assemblyPath, &params, &ctx);
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

    void InitHostFXRWithConfig(HStringView8 dotnetRoot, const char_t* configPath, load_assembly_and_get_function_pointer_fn& outLoadAssemblyFunc)
    {
        hostfxr_initialize_parameters params{};
        params.size = sizeof(hostfxr_initialize_parameters);
        #ifdef HE_PLATFORM_WINDOWS
        HString16 root = dotnetRoot.ToUTF16();
        params.dotnet_root = (const wchar_t*)root.Data();
        #else
        params.dotnet_root = dotnetRoot.Data();
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

    void ScriptingEngine::FindDotnetInstallations()
    {
        // Does not need to be set
        #ifdef HE_PLATFORM_WINDOWS
        HString16 root = HString8(std::getenv("DOTNET_SDK")).ToUTF16();
        #else
        HString8 root(std::getenv("DOTNET_SDK"));
        #endif

        const auto resultFn = [](
            const struct hostfxr_dotnet_environment_info* info,
            void* result_context
        ) {
            HString8 bestVersion;
            HString8 bestPath;
            for (u32 i = 0; i < info->sdk_count; i++)
            {
                #ifdef HE_PLATFORM_WINDOWS
                HString8 version = HString16((const char16_t*)info->sdks[i].version).ToUTF8();
                HString8 path = HString16((const char16_t*)info->sdks[i].path).ToUTF8();
                #else
                HString8 version((const char*)info->sdks[i].version);
                HString8 path((const char*)info->sdks[i].path);
                #endif

                HE_ENGINE_LOG_TRACE("Found dotnet sdk {} @ {}", version.Data(), path.Data());

                // TODO: don't hardcode this somehow
                auto nums = version.Split(".");
                if (nums[0] != "8") continue;

                // TODO: pick latest version, for now this is fine
                bestVersion = version;
                bestPath = path;

                break;
            }

            // Set the actual path to the root dotnet executable rather than the SDK location itself
            s_DotnetPath = std::filesystem::path(bestPath.Data()).parent_path().parent_path().u8string();

            HE_ENGINE_LOG_INFO("Using dotnet executable @ {}", s_DotnetPath.Data());
        };

        s_GetEnvInfoFunc(
            nullptr,//(const char_t*)root.Data(),
            nullptr,
            resultFn,
            nullptr
        );
    }

    bool ScriptingEngine::Initialize()
    {
        bool result = LoadHostFXR();
        if (!result)
        {
            HE_ENGINE_LOG_ERROR("Failed to load hostfxr");
            return false;
        }

        #ifdef HE_PLATFORM_ANDROID
            HString8 bridgePathStr(AndroidApp::App->activity->internalDataPath);
            bridgePathStr += "/BridgeScripts.dll";
            const char_t* bridgePath = bridgePathStr.Data();
            HE_ENGINE_LOG_TRACE("Loading BridgeScripts.dll from @ {0}", bridgePathStr.Data());
        #else
            const char_t* bridgePath = HOSTFXR_STR("scripting/BridgeScripts.dll");
        #endif

        load_assembly_and_get_function_pointer_fn loadAssemblyWithPtrFunc = nullptr;
        #ifdef HE_DIST
            InitHostFXRForCmdline(bridgePath, loadAssemblyWithPtrFunc);
        #else
            FindDotnetInstallations();

            if (s_DotnetPath.IsEmpty())
            {
                HE_ENGINE_LOG_ERROR("Unable to locate suitable dotnet installations");
                return false;
            }

            InitHostFXRWithConfig(
                s_DotnetPath,
                HOSTFXR_STR("scripting/BridgeScripts.runtimeconfig.json"),
                loadAssemblyWithPtrFunc
            );
        #endif

        if (!loadAssemblyWithPtrFunc)
        {
            HE_ENGINE_LOG_ERROR("Failed to initialize hostfxr");
            return false;
        }

        HE_ENGINE_LOG_DEBUG(".NET hostfxr initialized");

        InitializeFn initFunc = nullptr;
        int rc = loadAssemblyWithPtrFunc(
            bridgePath,
            HOSTFXR_STR("BridgeScripts.EntryPoint, BridgeScripts"),
            HOSTFXR_STR("Initialize"),
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            (void**)&initFunc
        );

        if (rc != 0)
        {
            HE_ENGINE_LOG_ERROR("Failed to load scripting entrypoint func");
            return false;
        }

        if (!initFunc(PlatformUtils::GetCurrentModuleHandle(), &s_BridgeCallbacks))
        {
            HE_ENGINE_LOG_ERROR("Failed to initialize bridge scripts");
            return false;
        }

        if (!s_BridgeCallbacks.EntryPoint_LoadCorePlugin(&s_CoreCallbacks))
        {
            HE_ENGINE_LOG_ERROR("Failed to load core plugin");
        }

        s_CoreCallbacks.UnmanagedCallbacks_PopulateCallbacks(NativeCallbacks);

        HE_ENGINE_LOG_DEBUG("Scripts ready");

        return true;
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
        HE_ENGINE_LOG_TRACE("Loading client plugin");

        auto timer = Timer("Client plugin load"); 

        if (s_ClientPluginLoaded)
        {
            bool res = UnloadClientPlugin();
            if (!res) return false;
        }

        bool res = s_BridgeCallbacks.EntryPoint_LoadClientPlugin(absolutePath.Data());
        if (!res)
        {
            HE_ENGINE_LOG_ERROR("Failed to load client plugin");
            return false;
        }

        HE_ENGINE_LOG_TRACE("Client plugin loaded from bridge");

        HArray outArgs;
        s_CoreCallbacks.PluginReflection_GetClientInstantiableClasses(&outArgs);

        auto populate = [&outArgs](u32 index, std::unordered_map<s64, ScriptClass>& target)
        {
            auto classes = outArgs[index * 2].Array();
            auto ids = outArgs[index * 2 + 1].Array();
            for (u32 i = 0; i < classes.Count(); i++)
            {
                auto convertedString = classes[i].String().Convert(HString::Encoding::UTF8);
                s64 id = ids[i].Int();
                s_NameToId[convertedString] = id;
                target[id] = ScriptClass(convertedString, 0);
            }
        };

        // Populate entity classes
        populate(0, s_EntityClasses);

        // Populate component classes
        populate(1, s_ComponentClasses);

        HE_ENGINE_LOG_INFO("Client plugin fully loaded");
        s_ClientPluginLoaded = true;
        return true;
    }

    bool ScriptingEngine::UnloadClientPlugin()
    {
        if (!s_ClientPluginLoaded) return true;

        s_NameToId.clear();
        s_EntityClasses.clear();
        s_ComponentClasses.clear();

        bool res = s_BridgeCallbacks.EntryPoint_UnloadClientPlugin();
        if (!res)
        {
            HE_ENGINE_LOG_ERROR("Failed to unload client plugin");
            return false;
        }

        HE_ENGINE_LOG_INFO("Client plugin successfully unloaded");
        s_ClientPluginLoaded = false;
        return true;
    }

    bool ScriptingEngine::ReloadCorePlugin()
    {
        auto timer = Timer("Core plugin reload"); 

        bool res = UnloadClientPlugin();
        if (!res)
        {
            HE_ENGINE_LOG_ERROR("Failed to unload client plugin while reloading core plugin");
            return false;
        }

        res = s_BridgeCallbacks.EntryPoint_UnloadCorePlugin();
        if (!res)
        {
            HE_ENGINE_LOG_ERROR("Failed to unload core plugin while reloading");
            return false;
        }

        res = s_BridgeCallbacks.EntryPoint_LoadCorePlugin(&s_CoreCallbacks);
        if (!res)
        {
            HE_ENGINE_LOG_ERROR("Failed to load core plugin while reloading");
            return false;
        }

        s_CoreCallbacks.UnmanagedCallbacks_PopulateCallbacks(NativeCallbacks);

        HE_ENGINE_LOG_INFO("Core plugin successfully reloaded");
        return true;
    }

    uptr ScriptingEngine::InstantiateScriptComponent(const HString& type)
    {
        HE_PROFILE_FUNCTION();

        uptr result = (uptr)s_CoreCallbacks.ManagedObject_InstantiateClientScriptComponent(&type);
        if (result == 0)
            HE_ENGINE_LOG_ERROR("Failed to instantiate class '{0}'", type.ToUTF8().Data());
        
        return result;
    }

    uptr ScriptingEngine::InstantiateScriptEntity(const HString& type, u32 entityHandle, Scene* sceneHandle)
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

    bool ScriptingEngine::IsClassIdInstantiable(s64 classId)
    {
        return s_EntityClasses.find(classId) != s_EntityClasses.end() ||
               s_ComponentClasses.find(classId) != s_ComponentClasses.end();
    }

    s64 ScriptingEngine::GetClassIdFromName(const HString& name)
    {
        auto found = s_NameToId.find(name);
        if (found == s_NameToId.end())
            return 0;
        return found->second;
    }
}
