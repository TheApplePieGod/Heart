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

    public:
        static Heart::Ref<Project> CreateAndLoad(const Heart::HStringView8& absolutePath, const Heart::HStringView8& name);
        static Heart::Ref<Project> LoadFromPath(const Heart::HStringView8& absolutePath);

        static Project* GetActiveProject() { return s_ActiveProject.get(); }

    private:
        inline static Heart::Ref<Project> s_ActiveProject = nullptr;
        
    private:
        Heart::HString8 m_Name;
        Heart::HString8 m_AbsolutePath;

        friend class Widgets::ProjectSettings;
    };
}