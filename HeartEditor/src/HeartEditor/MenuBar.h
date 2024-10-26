#pragma once

#include "Heart/Container/HString8.h"

namespace HeartEditor
{
    class MenuBar
    {
    public:
        MenuBar() = default;

        void OnImGuiRender();

        static bool RenderNewProjectDialog(
            bool openDialog,
            Heart::HString8& newPath,
            Heart::HString8& newName
        );

    private:
        Heart::HString8 m_NewProjectPath;
        Heart::HString8 m_NewProjectName;
    };
}
