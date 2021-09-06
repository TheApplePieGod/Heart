#pragma once

namespace HeartEditor
{
namespace Widgets
{
    class MenuBar
    {
    public:
        MenuBar();

        void OnImGuiRender();

        inline bool* GetWindowStatusRef(const std::string& name) { return &m_WindowStatuses[name]; }
        inline bool GetWindowStatus(const std::string& name) { return m_WindowStatuses[name]; }

    private:
        std::unordered_map<std::string, bool> m_WindowStatuses;
    };
}
}