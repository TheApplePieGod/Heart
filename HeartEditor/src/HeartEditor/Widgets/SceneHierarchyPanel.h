#pragma once

#include "HeartEditor/Util/VirtualizedTree.h"
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
        void CreateEmptyEntity();

    private:
        Heart::HVector<std::pair<Heart::HStringView, entt::entity>> m_SortingPairs;
        Heart::HVector<VirtualizedTree::Node> m_RootNodes;
        VirtualizedTree m_Tree;
    };
}
}
