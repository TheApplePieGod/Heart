#pragma once

#include "Heart/Container/HString8.h"

namespace HeartEditor
{
    enum class ExportPlatform : u8
    {
        None = 0,
        Windows,
        MacOS,
        Linux,
        Android,

#ifdef HE_PLATFORM_WINDOWS
        CurrentPlatform = Windows
#elif defined(HE_PLATFORM_MACOS)
        CurrentPlatform = MacOS
#elif defined(HE_PLATFORM_LINUX)
        CurrentPlatform = Linux
#elif defined(HE_PLATFORM_ANDROID)
        CurrentPlatform = Android
#endif
    };

    namespace Widgets { class ProjectSettings; }
    class Project
    {
    public:
        Project(const Heart::HStringView8& absolutePath)
            : m_AbsolutePath(absolutePath)
        {}

        void SetActive();
        void SaveToDisk();
        void LoadScriptsPlugin();
        bool BuildScripts(bool debug);
        bool PublishScripts(ExportPlatform platform);
        void Export(Heart::HStringView8 absolutePath, ExportPlatform platform);
        
        inline Heart::HStringView8 GetPath() const { return m_AbsolutePath; }
        inline Heart::HStringView8 GetName() const { return m_Name; }
        
    public:
        static Heart::Ref<Project> CreateAndLoad(const Heart::HStringView8& absolutePath, const Heart::HStringView8& name);
        static Heart::Ref<Project> LoadFromPath(const Heart::HStringView8& absolutePath);

    private:
        struct SerializedInstance
        {
            nlohmann::json ScriptComp;
            std::unordered_map<s64, nlohmann::json> RuntimeComps;
        };

    private:
        static void CopyTemplateDirectory(
            std::filesystem::path src,
            std::filesystem::path dst,
            const Heart::HStringView8& scriptsRoot,
            const Heart::HStringView8& projectName
        );

    private:
        Heart::HString8 RunCommandInProjectDirectory(Heart::HStringView8 command, int& outResult);
        Heart::HStringView8 GetDotnetRuntimeIdentifier(ExportPlatform platform);
        
    private:
        Heart::HString8 m_Name;
        Heart::HString8 m_AbsolutePath;

        friend class Widgets::ProjectSettings;
    };
}
