#pragma once

namespace HeartEditor
{
    namespace Widgets { class ProjectSettings; }
    class Project
    {
    public:
         Project::Project(const std::string& absolutePath)
            : m_AbsolutePath(absolutePath)
        {}

        void SetActive();
        void SaveToDisk();
        void LoadClientAssembly();

    public:
        static Heart::Ref<Project> CreateAndLoad(const std::string& absolutePath, const std::string& name);
        static Heart::Ref<Project> LoadFromPath(const std::string& absolutePath);

        static Project* GetActiveProject() { return s_ActiveProject.get(); }

    private:
        static Heart::Ref<Project> s_ActiveProject;
        
    private:
        std::string m_Name;
        std::string m_AbsolutePath;

        friend class Widgets::ProjectSettings;
    };
}