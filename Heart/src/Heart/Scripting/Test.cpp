#include "hepch.h"
#include "Test.h"

#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
#include "mono/metadata/debug-helpers.h"
#include "mono/metadata/mono-debug.h"

namespace Heart
{
    #define HE_MONO_CHECK_RESULT(val, msg) if (!val) { HE_ENGINE_LOG_ERROR(msg); return; } 
    
    void Log_Native(int level, MonoString* message)
    {
        Logger::GetClientLogger().log(spdlog::source_loc{}, static_cast<spdlog::level::level_enum>(level), mono_string_to_utf8(message));
    }

    void ScriptTest::Test()
    {
        std::array<const char*, 2> options = {
            "--soft-breakpoints",
            "--debugger-agent=transport=dt_socket,server=y,address=127.0.0.1:55555,suspend=y,loglevel=10"
        };
        // mono_jit_parse_options(options.size(), (char**)options.data());
        mono_debug_init(MONO_DEBUG_FORMAT_MONO);
        mono_set_dirs(".", ".");

        MonoDomain* domain = mono_jit_init("HeartEngine");
        HE_MONO_CHECK_RESULT(domain, "Failed to create c# domain");

        MonoAssembly* assembly = mono_domain_assembly_open(domain, "ScriptingCore.dll");
        HE_MONO_CHECK_RESULT(assembly, "Failed to load c# assembly");

        MonoImage* image = mono_assembly_get_image(assembly);
        HE_MONO_CHECK_RESULT(image, "Failed to load c# assembly image");

        mono_add_internal_call(
            "Heart.Core.Log::Log_Native(Heart.Core.Log/Level,string)",
            &Log_Native
        );

        MonoClass* testClass = mono_class_from_name(image, "Heart", "Class1");
        HE_MONO_CHECK_RESULT(testClass, "Failed to load c# test class");

        MonoMethodDesc* mainMethodDesc = mono_method_desc_new("Heart.Class1::main()", true);
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
        }

        mono_jit_cleanup(domain);
        mono_debug_cleanup();
    }
}