#pragma once

#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"

namespace HeartEditor
{
namespace Widgets
{
    class MenuBar
    {
    public:
        MenuBar();

        void OnImGuiRender(Heart::Ref<Heart::Scene>& activeScene, Heart::Entity& selectedEntity);

        inline bool* GetWindowOpenRef(const std::string& name) { return &m_WindowStatuses[name].Open; }
        inline bool IsWindowOpen(const std::string& name) { return m_WindowStatuses[name].Open; }
        inline bool* GetWindowDirtyRef(const std::string& name) { return &m_WindowStatuses[name].Dirty; }
        inline bool IsWindowDirty(const std::string& name) { return m_WindowStatuses[name].Dirty; }

        inline void SetWindowOpen(const std::string& name, bool open) { m_WindowStatuses[name].Open = open; }
        inline void SetWindowDirty(const std::string& name, bool dirty) { m_WindowStatuses[name].Dirty = dirty; }

        bool AreAnyWindowsDirty();

    private:
        struct WindowStatus
        {
            bool Open = false;
            bool Dirty = false;
        };

    private:
        std::unordered_map<std::string, WindowStatus> m_WindowStatuses;
    };
}
}