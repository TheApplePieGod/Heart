#pragma once

#include "Heart/Container/HString.h"

namespace HeartEditor
{
    namespace Widgets { class ProjectSettings; }
    class Project
    {
    public:
         Project::Project(const Heart::HStringView& absolutePath)
            : m_AbsolutePath(absolutePath)
        {}

        void SetActive();
        void SaveToDisk();
        void LoadScriptsPlugin();

    public:
        static Heart::Ref<Project> CreateAndLoad(const Heart::HStringView& absolutePath, const Heart::HStringView& name);
        static Heart::Ref<Project> LoadFromPath(const Heart::HStringView& absolutePath);

        static Project* GetActiveProject() { return s_ActiveProject.get(); }

    private:
        inline static Heart::Ref<Project> s_ActiveProject = nullptr;
        
    private:
        Heart::HString m_Name;
        Heart::HString m_AbsolutePath;

        friend class Widgets::ProjectSettings;
    };
}