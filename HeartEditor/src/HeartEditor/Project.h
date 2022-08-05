#pragma once

#include "Heart/Container/HString.h"

namespace HeartEditor
{
    namespace Widgets { class ProjectSettings; }
    class Project
    {
    public:
         Project::Project(const Heart::HString& absolutePath)
            : m_AbsolutePath(absolutePath)
        {}

        void SetActive();
        void SaveToDisk();
        void LoadScriptsPlugin();

    public:
        static Heart::Ref<Project> CreateAndLoad(const Heart::HString& absolutePath, const Heart::HString& name);
        static Heart::Ref<Project> LoadFromPath(const Heart::HString& absolutePath);

        static Project* GetActiveProject() { return s_ActiveProject.get(); }

    private:
        inline static Heart::Ref<Project> s_ActiveProject = nullptr;
        
    private:
        Heart::HString m_Name;
        Heart::HString m_AbsolutePath;

        friend class Widgets::ProjectSettings;
    };
}