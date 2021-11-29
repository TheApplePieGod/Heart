#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"

namespace HeartEditor
{
namespace Widgets
{
    class SceneHierarchyPanel : public Widget
    {
    public:
        SceneHierarchyPanel(const std::string& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRender() override;

    private:
        void RenderEntity(entt::entity entity);
    };
}
}