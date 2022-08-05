#pragma once

#include "Heart/Container/HString.h"

namespace HeartEditor
{
    class MenuBar
    {
    public:
        MenuBar() = default;

        void OnImGuiRender();

    private:
        Heart::HString m_NewProjectPath;
        Heart::HString m_NewProjectName;
    };
}