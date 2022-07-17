#pragma once

struct _MonoDomain;
struct _MonoImage;
namespace Heart
{
    class ScriptingEngine
    {
    public:
        static void Initialize();
        static void Shutdown();

        static bool LoadClientAssembly(const std::string& absolutePath);
        static void UnloadClientAssembly();

        static bool Test();

    private:
        inline static _MonoDomain* s_CoreDomain = nullptr;
        inline static _MonoImage* s_CoreImage = nullptr;
        inline static _MonoDomain* s_ClientDomain = nullptr;
        inline static _MonoImage* s_ClientImage = nullptr;
    };
}

#define HE_MONO_ASSERT_RESULT(val, msg) HE_ENGINE_ASSERT(val, msg);
#define HE_MONO_CHECK_RESULT(val, msg) if (!val) { HE_ENGINE_LOG_ERROR(msg); return false; }