#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "Heart/Scene/Entity.h"

namespace HeartEditor
{
namespace Widgets
{
    class SceneHierarchyPanel : public Widget
    {
    public:
        SceneHierarchyPanel(const Heart::HStringView8& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRender() override;

    private:
        bool RenderEntity(entt::entity entity);
    };
}
}