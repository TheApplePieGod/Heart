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

        void OnImGuiRender(Heart::Scene* activeScene);

        inline Heart::Entity GetSelectedEntity() const { return m_SelectedEntity; }
        inline void SetSelectedEntity(Heart::Entity entity) { m_SelectedEntity = entity; }

    private:
        Heart::Entity m_SelectedEntity;
    };
}
}