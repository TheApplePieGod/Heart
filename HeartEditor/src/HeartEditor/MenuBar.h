#pragma once

namespace HeartEditor
{
    class MenuBar
    {
    public:
        MenuBar() = default;

        void OnImGuiRender();

    private:
        std::string m_NewProjectPath;
        std::string m_NewProjectName;
    };
}