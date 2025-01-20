#pragma once

#include "HeartEditor/Editor.h"

namespace HeartEditor
{
    class ProjectPicker
    {
    public:
        ProjectPicker() = default;

        void OnImGuiRender();

        inline const ProjectDescriptor& GetSelectedProject() const { return m_SelectedProject; }

    private:
        ProjectDescriptor m_SelectedProject;
        Heart::HString8 m_NewProjectPath;
        Heart::HString8 m_NewProjectName;
    };
}
