#include "hepch.h"
#include "RenderGraph.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "HeartEditor/EditorCamera.h"
#include "Heart/Input/Input.h"
#include "Heart/Core/Timing.h"
#include "Heart/Renderer/RenderPlugin.h"
#include "imgui/imgui.h"

#include "HeartEditor/Widgets/Viewport.h"
#include "imgui_node_editor.h"

namespace NodeEditor = ax::NodeEditor;

namespace HeartEditor
{
namespace Widgets
{
    RenderGraph::RenderGraph(const Heart::HStringView8& name, bool initialOpen)
        : Widget(name, initialOpen)
    {
        m_EditorContext = NodeEditor::CreateEditor();
    }

    RenderGraph::~RenderGraph()
    {
        NodeEditor::DestroyEditor(m_EditorContext);
    }

    void RenderGraph::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        NodeEditor::SetCurrentEditor(m_EditorContext);
        NodeEditor::Begin("RGEditor");

        auto& viewport = (Widgets::Viewport&)Editor::GetWindow("Viewport");
        auto& renderer = viewport.GetSceneRenderer2();
        m_RendererContext = &renderer;
        m_IdCounter = 0;

        // Create a node for each plugin
        float yPos = 0.f;
        for (const auto& leaf : renderer.GetPluginLeaves())
        {
            RenderNode(leaf, yPos);
            yPos += 75.f;
        }
        NodeEditor::End();
        NodeEditor::SetCurrentEditor(nullptr);
        
        ImGui::End();
        ImGui::PopStyleVar();

        m_FirstRender = false;
    }

    void RenderGraph::RenderNode(Heart::Ref<Heart::RenderPlugin> plugin, float yPos)
    {
        const auto& plugins = m_RendererContext->GetPlugins();

        NodeEditor::BeginNode((u64)plugin->GetUUID());

        const float nodeWidth = 200.f;
        ImVec2 horizLayoutSize = { nodeWidth, 0.f };

        ImGui::Text(plugin->GetName().Data());
        ImGui::BeginHorizontal(plugin->GetName().Data(), horizLayoutSize);

        // Add dependencies as incoming pins
        // Should have no issue with unique ids here
        u64 idCounter = plugin->GetUUID();
        ImGui::BeginVertical("in");
        for (const auto& dep : plugin->GetDependencies())
        {
            NodeEditor::BeginPin(++idCounter, NodeEditor::PinKind::Input);
            ImGui::Text(">");
            NodeEditor::EndPin();
        }
        ImGui::EndVertical();

        ImGui::Spring(1.0f, 0);

        ImGui::BeginVertical("out");
        for (const auto& depName : plugin->GetDependents())
        {
            // Find index of this node
            u32 idx = 0;
            const auto& dep = m_RendererContext->GetPlugin(depName);
            for (; idx < dep->GetDependencies().Count(); idx++)
                if (dep->GetDependencies()[idx].get() == plugin.get())
                    break;

            NodeEditor::BeginPin(++idCounter, NodeEditor::PinKind::Output);
            ImGui::Text(">");
            NodeEditor::EndPin();

            NodeEditor::Link(++m_IdCounter, idCounter, dep->GetUUID() + idx + 1);
            NodeEditor::Flow(m_IdCounter, NodeEditor::FlowDirection::Forward);
        }
        ImGui::EndVertical();

        ImGui::EndHorizontal();
        NodeEditor::EndNode();

        // Set x position based on computed depth
        float xOffset = 75.f;
        float xPos = plugin->GetMaxDepth() * (nodeWidth + xOffset);
        if (m_FirstRender)
            NodeEditor::SetNodePosition((u64)plugin->GetUUID(), { xPos, yPos });

        // Recurse
        for (const auto& dep : plugin->GetDependencies())
            RenderNode(dep, yPos);
    }
}
}
