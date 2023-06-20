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
        auto& renderer = viewport.GetSceneRenderer();
        m_RendererContext = &renderer;
        m_IdCounter = 0;

        // Create a node for each plugin
        float yPos = 0.f;
        for (const auto& leaf : renderer.GetGraphData(Heart::GraphDependencyType::GPU).Leaves)
        {
            RenderNode(leaf, yPos, Heart::GraphDependencyType::GPU);
            yPos += 75.f;
        }
        NodeEditor::End();
        NodeEditor::SetCurrentEditor(nullptr);
        
        ImGui::End();
        ImGui::PopStyleVar();

        m_FirstRender = false;
    }

    void RenderGraph::RenderNode(const Heart::HString8& pluginName, float yPos, Heart::GraphDependencyType depType)
    {
        auto plugin = m_RendererContext->GetPlugin(pluginName);
        const auto& graphData = plugin->GetGraphData(depType);

        NodeEditor::BeginNode((u64)plugin->GetUUID());

        const float nodeWidth = 200.f;
        ImVec2 horizLayoutSize = { nodeWidth, 0.f };

        ImGui::Text(pluginName.Data());
        ImGui::BeginHorizontal(plugin->GetName().Data(), horizLayoutSize);

        // Add dependencies as incoming pins
        // Should have no issue with unique ids here
        u64 idCounter = plugin->GetUUID();
        ImGui::BeginVertical("in");
        for (const auto& dep : graphData.Dependencies)
        {
            NodeEditor::BeginPin(++idCounter, NodeEditor::PinKind::Input);
            ImGui::Text(">");
            NodeEditor::EndPin();
        }
        ImGui::EndVertical();

        ImGui::Spring(1.0f, 0);

        ImGui::BeginVertical("out");
        for (const auto& depName : graphData.Dependents)
        {
            // Find index of this node
            u32 idx = 0;
            const auto& dep = m_RendererContext->GetPlugin(depName);
            const auto& dependencies = dep->GetGraphData(depType).Dependencies;
            for (auto& name : dependencies)
            {
                if (name == pluginName)
                    break;
                idx++;
            }

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
        float xPos = graphData.MaxDepth * (nodeWidth + xOffset);
        if (m_FirstRender)
            NodeEditor::SetNodePosition((u64)plugin->GetUUID(), { xPos, yPos });

        // Recurse
        for (const auto& dep : graphData.Dependencies)
            RenderNode(dep, yPos, depType);
    }
}
}
