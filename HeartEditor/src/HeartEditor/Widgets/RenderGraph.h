#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "imgui_node_editor.h"

namespace HeartEditor
{
namespace Widgets
{
    class RenderGraph : public Widget
    {
    public:
        RenderGraph(const Heart::HStringView8& name, bool initialOpen);
        ~RenderGraph();

        void OnImGuiRender() override;

    private:
        void RenderNode(const Heart::HString8& pluginName, Heart::GraphDependencyType depType);

    private:
        ax::NodeEditor::EditorContext* m_EditorContext;

        Heart::SceneRenderer* m_RendererContext;
        Heart::GraphDependencyType m_ViewType = Heart::GraphDependencyType::CPU;
        u64 m_IdCounter = 0;
        bool m_FirstRender = true;
        std::unordered_map<u32, float> m_DepthOffsets;
    };
}
}
