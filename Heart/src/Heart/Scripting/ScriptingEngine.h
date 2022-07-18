#pragma once

struct _MonoAssembly;
struct _MonoDomain;
struct _MonoImage;
struct _MonoObject;
namespace Heart
{
    struct AssemblyClassEntry
    {
        AssemblyClassEntry(const char* _namespace, const char* _class, const char* _fullName)
            : Namespace(_namespace), Class(_class), FullName(_fullName)
        {}

        std::string Namespace;
        std::string Class;
        std::string FullName;
    };

    class ScriptingEngine
    {
    public:
        static void Initialize();
        static void Shutdown();

        static bool LoadClientAssembly(const std::string& absolutePath);
        static void UnloadClientAssembly();
        static _MonoObject* InstantiateClass(const std::string& namespaceName, const std::string& className);

        inline static const std::vector<AssemblyClassEntry>& GetAssemblyClasses() { return s_AssemblyClasses; }

    private:
        static _MonoAssembly* LoadAssembly(const std::string& absolutePath);
        static void LoadAssemblyClasses();

    private:
        inline static _MonoDomain* s_CoreDomain = nullptr;
        inline static _MonoImage* s_CoreImage = nullptr;
        inline static _MonoDomain* s_ClientDomain = nullptr;
        inline static _MonoImage* s_ClientImage = nullptr;

        inline static std::vector<AssemblyClassEntry> s_AssemblyClasses;
    };
}

#define HE_MONO_CHECK_RESULT(val, msg) HE_ENGINE_ASSERT(val, msg);