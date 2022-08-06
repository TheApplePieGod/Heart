#pragma once

#include "Heart/Container/HString8.h"

namespace HeartEditor
{
    class MenuBar
    {
    public:
        MenuBar() = default;

        void OnImGuiRender();

    private:
        Heart::HString8 m_NewProjectPath;
        Heart::HString8 m_NewProjectName;
    };
}