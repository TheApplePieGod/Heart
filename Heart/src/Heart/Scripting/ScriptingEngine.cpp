#include "hepch.h"
#include "ScriptingEngine.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Scripting/InternalCalls.h"
#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
#include "mono/metadata/debug-helpers.h"
#include "mono/metadata/mono-debug.h"

namespace Heart
{
    void ScriptingEngine::Initialize()
    {
        std::array<const char*, 2> options = {
            "--soft-breakpoints",
            "--debugger-agent=transport=dt_socket,server=y,address=127.0.0.1:55555,suspend=y,loglevel=10"
        };
        // mono_jit_parse_options(options.size(), (char**)options.data()); // Debug enabled
        mono_debug_init(MONO_DEBUG_FORMAT_MONO);
        mono_set_dirs(".", ".");

        // Init core domain
        s_CoreDomain = mono_jit_init("CoreDomain");
        HE_MONO_CHECK_RESULT(s_CoreDomain, "Failed to create core c# domain");

        // Load core assembly
        MonoAssembly* assembly = mono_domain_assembly_open(s_CoreDomain, "CoreScripts.dll");
        HE_MONO_CHECK_RESULT(assembly, "Failed to load core c# assembly");

        // Get core image
        s_CoreImage = mono_assembly_get_image(assembly);
        HE_MONO_CHECK_RESULT(s_CoreImage, "Failed to get core c# assembly image");

        InternalCalls::Map();
    }

    void ScriptingEngine::Shutdown()
    {
        mono_jit_cleanup(s_CoreDomain);
        mono_debug_cleanup();
    }

    bool ScriptingEngine::LoadClientAssembly(const std::string& absolutePath)
    {
        if (s_ClientDomain) UnloadClientAssembly();

        // Create new client domain
        s_ClientDomain = mono_domain_create_appdomain("ClientDomain", nullptr);
        if (!s_ClientDomain)
        {
            HE_ENGINE_LOG_ERROR("Failed to create client c# domain");
            return false;
        }

        // Set this new domain as active
        mono_domain_set(s_ClientDomain, false);

        // Load client assembly
        MonoAssembly* assembly = LoadAssembly(absolutePath);
        if (!assembly)
        {
            HE_ENGINE_LOG_ERROR("Failed to load client c# assembly");
            return false;
        }

        // Get client image
        s_ClientImage = mono_assembly_get_image(assembly);
        if (!s_ClientImage)
        {
            HE_ENGINE_LOG_ERROR("Failed to get client c# image");
            return false;
        }

        LoadAssemblyClasses();

        return true;
    }

    void ScriptingEngine::UnloadClientAssembly()
    {
        if (!s_ClientDomain) return;

        // Set active domain back to core to allow unloading
        mono_domain_set(s_CoreDomain, false);
        
        // Unload the domain
        mono_domain_unload(s_ClientDomain);

        s_ClientDomain = nullptr;
    }

    MonoObject* ScriptingEngine::InstantiateClass(const std::string& namespaceName, const std::string& className, u32* outGCHandle)
    {
        // Load class
        MonoClass* classObj = mono_class_from_name(s_ClientImage, namespaceName.c_str(), className.c_str());
        if (!classObj)
        {
            HE_ENGINE_LOG_ERROR("Failed to locate class '{0}.{1}'", namespaceName, className);
            return nullptr;
        }

        // Create new instance
        MonoObject* instance = mono_object_new(s_ClientDomain, classObj);
        if (!instance)
        {
            HE_ENGINE_LOG_ERROR("Failed to instantiate class '{0}.{1}'", namespaceName, className);
            return nullptr;
        }

        // Call default constructor on instance if exists
        // TODO: cache this value?
        if (mono_class_get_method_from_name(classObj, ".ctor", 0))
            mono_runtime_object_init(instance);

        // Create GC handle for this object to prevent garbage collection
        if (outGCHandle)
            *outGCHandle = mono_gchandle_new(instance, false);

        return instance;
    }

    void ScriptingEngine::FreeObjectHandle(u32 gcHandle)
    {
        mono_gchandle_free(gcHandle);
    }

    MonoAssembly* ScriptingEngine::LoadAssembly(const std::string& absolutePath)
    {
        u32 dataLength = 0;
        unsigned char* data = FilesystemUtils::ReadFile(absolutePath, dataLength);

        MonoImageOpenStatus status;
        MonoImage* image = mono_image_open_from_data_full((char*)data, dataLength, true, &status, false);

        if (status != MONO_IMAGE_OK)
        {
            const char* error = mono_image_strerror(status);
            HE_ENGINE_LOG_ERROR("Failed to read assembly: {0}", error);
            return nullptr;
        }

        MonoAssembly* assembly = mono_assembly_load_from_full(image, absolutePath.c_str(), &status, false);
        mono_image_close(image);

        delete[] data;
        return assembly;
    }

    void ScriptingEngine::LoadAssemblyClasses()
    {
        s_AssemblyClasses.clear();
        const MonoTableInfo* table = mono_image_get_table_info(s_ClientImage, MONO_TABLE_TYPEDEF);
        int numRows = mono_table_info_get_rows(table);

        for (int i = 0; i < numRows; i++)
        {
            u32 cols[MONO_TYPEDEF_SIZE];
            mono_metadata_decode_row(table, i, cols, MONO_TYPEDEF_SIZE);

            const char* namespaceName = mono_metadata_string_heap(s_ClientImage, cols[MONO_TYPEDEF_NAMESPACE]);
            const char* className = mono_metadata_string_heap(s_ClientImage, cols[MONO_TYPEDEF_NAME]);

            char fullName[100];
            sprintf(fullName, "%s.%s", namespaceName, className);

            if (namespaceName[0] != 0)
                s_AssemblyClasses.emplace_back(namespaceName, className, fullName);
        }
    }

    // void ScriptingEngine::Test()
    // {
    //     auto obj = InstantiateClass("TestingProject.Scripts", "Class1");

    //     MonoObject* exception = nullptr;
    //     mono_runtime_invoke(mainMethod, nullptr, nullptr, &exception);

    //     // Report exception
    //     if (exception)
    //     {
    //         MonoString* exString = mono_object_to_string(exception, nullptr);
    //         HE_ENGINE_LOG_ERROR("Failed to invoke c# main: {0}", mono_string_to_utf8(exString));
    //         return false;
    //     }

    //     return true;
    // }
}