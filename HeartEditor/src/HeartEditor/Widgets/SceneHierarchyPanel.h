#pragma once

#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"

namespace HeartEditor
{
namespace Widgets
{
    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel();

        void OnImGuiRender(Heart::Scene* activeScene, Heart::Entity& selectedEntity);

    private:
        void RenderEntity(Heart::Scene* activeScene, entt::entity entity, Heart::Entity& selectedEntity);
    };
}
}