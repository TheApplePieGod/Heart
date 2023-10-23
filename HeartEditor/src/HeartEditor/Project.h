#pragma once

#include "Heart/Container/HString8.h"

namespace HeartEditor
{
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
        void Export(Heart::HStringView8 absolutePath);
        
        inline Heart::HStringView8 GetPath() const { return m_AbsolutePath; }
        
    public:
        static Heart::Ref<Project> CreateAndLoad(const Heart::HStringView8& absolutePath, const Heart::HStringView8& name);
        static Heart::Ref<Project> LoadFromPath(const Heart::HStringView8& absolutePath);

        static Project* GetActiveProject() { return s_ActiveProject.get(); }

    private:
        struct SerializedInstance
        {
            nlohmann::json ScriptComp;
            std::unordered_map<s64, nlohmann::json> RuntimeComps;
        };

    private:
        inline static Heart::Ref<Project> s_ActiveProject = nullptr;
        
    private:
        Heart::HString8 m_Name;
        Heart::HString8 m_AbsolutePath;

        friend class Widgets::ProjectSettings;
    };
}
