#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "Heart/Renderer/SceneRenderer2.h"
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
        void RenderNode(Heart::Ref<Heart::RenderPlugin> plugin, float yPos);

    private:
        ax::NodeEditor::EditorContext* m_EditorContext;

        Heart::SceneRenderer2* m_RendererContext;
        u64 m_IdCounter = 0;
        bool m_FirstRender = true;
    };
}
}
