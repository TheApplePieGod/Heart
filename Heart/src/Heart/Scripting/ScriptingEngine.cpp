#include "hepch.h"
#include "ScriptingEngine.h"

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
        HE_MONO_ASSERT_RESULT(s_CoreDomain, "Failed to create core c# domain");

        // Load core assembly
        MonoAssembly* assembly = mono_domain_assembly_open(s_CoreDomain, "CoreScripts.dll");
        HE_MONO_ASSERT_RESULT(assembly, "Failed to load core c# assembly");

        // Get core image
        s_CoreImage = mono_assembly_get_image(assembly);
        HE_MONO_ASSERT_RESULT(s_CoreImage, "Failed to get core c# assembly image");

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
        HE_MONO_CHECK_RESULT(s_ClientDomain, "Failed to create client c# domain");

        // Load client assembly
        MonoAssembly* assembly = mono_domain_assembly_open(s_ClientDomain, absolutePath.c_str());
        HE_MONO_CHECK_RESULT(assembly, "Failed to load client c# assembly");

        // Get client image
        s_ClientImage = mono_assembly_get_image(assembly);
        HE_MONO_CHECK_RESULT(s_ClientImage, "Failed to get client c# assembly image");

        // Set this new domain as active
        mono_domain_set(s_ClientDomain, false);

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

    bool ScriptingEngine::Test()
    {
        MonoClass* testClass = mono_class_from_name(s_ClientImage, "TestingProject.Scripts", "Class1");
        HE_MONO_CHECK_RESULT(testClass, "Failed to load c# test class");

        MonoMethodDesc* mainMethodDesc = mono_method_desc_new("TestingProject.Scripts.Class1::main()", true);
        HE_MONO_CHECK_RESULT(mainMethodDesc, "Failed to load c# test class main method");

        MonoMethod* mainMethod = mono_method_desc_search_in_class(mainMethodDesc, testClass);
        HE_MONO_CHECK_RESULT(mainMethod, "Failed to load c# test class main method");
        mono_method_desc_free(mainMethodDesc);

        MonoObject* exception = nullptr;
        mono_runtime_invoke(mainMethod, nullptr, nullptr, &exception);

        // Report exception
        if (exception)
        {
            MonoString* exString = mono_object_to_string(exception, nullptr);
            HE_ENGINE_LOG_ERROR("Failed to invoke c# main: {0}", mono_string_to_utf8(exString));
            return false;
        }

        return true;
    }
}