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

        ImVec2 startPos = ImGui::GetCursorPos();
        ImGui::SetNextItemAllowOverlap();
        NodeEditor::SetCurrentEditor(m_EditorContext);
        NodeEditor::Begin("RGEditor");

        auto& viewport = (Widgets::Viewport&)Editor::GetWindow("Viewport");
        auto& renderer = viewport.GetSceneRenderer();
        m_RendererContext = &renderer;
        m_IdCounter = 0;

        // Render a node for each plugin in order of depth
        for (u32 i = 0; i <= renderer.GetGraphData(m_ViewType).MaxDepth; i++)
        {
            m_DepthOffsets.Add({ 0.f, 0.f });
            for (auto& pair : renderer.GetPlugins())
                if (pair.second->GetGraphData(m_ViewType).MaxDepth == i)
                    RenderNode(pair.first, m_ViewType);
        }

        // Render links once all nodes have been drawn
        for (auto& pair : renderer.GetPlugins())
        {
            auto& graphData = pair.second->GetGraphData(m_ViewType);
            for (u32 i = 0; i < graphData.Dependents.Count(); i++)
            {
                // Find index of this node
                u32 idx = 0;
                const auto& dep = m_RendererContext->GetPlugin(graphData.Dependents[i]);
                const auto& dependencies = dep->GetGraphData(m_ViewType).Dependencies;
                for (auto& name : dependencies)
                {
                    if (name == pair.first)
                        break;
                    idx++;
                }

                u64 outputId = pair.second->GetUUID() + i + graphData.Dependencies.size() + 1;
                NodeEditor::Link(++m_IdCounter, outputId, dep->GetUUID() + idx + 1);
                //NodeEditor::Flow(m_IdCounter, NodeEditor::FlowDirection::Forward);
            }
        }

        NodeEditor::End();
        NodeEditor::SetCurrentEditor(nullptr);

        m_FirstRender = false;

        // Need to do explicit hover check because overlapping is strange
        ImGui::SetCursorPos(startPos);
        ImGui::Button(m_ViewType == Heart::GraphDependencyType::CPU ? "CPU View" : "GPU View");
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_ViewType = m_ViewType == Heart::GraphDependencyType::CPU ? Heart::GraphDependencyType::GPU : Heart::GraphDependencyType::CPU;
            m_FirstRender = true;
            m_DepthOffsets.Clear();
        }
        
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void RenderGraph::RenderNode(const Heart::HString8& pluginName, Heart::GraphDependencyType depType)
    {
        auto plugin = m_RendererContext->GetPlugin(pluginName);
        const auto& graphData = plugin->GetGraphData(depType);

        NodeEditor::BeginNode((u64)plugin->GetUUID());

        Flourish::Texture* tex = plugin->GetOutputTexture().get();
        ImVec2 horizLayoutSize = { tex ? 0.f : 200.f , 0.f };

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

        if (tex)
        {
            float aspect = (float)tex->GetWidth() / (float)tex->GetHeight();
            ImGui::Image(
                tex->GetImGuiHandle(),
                { 500.f, std::max(150.f, 500.f / aspect) }
            );
        }
        else
            ImGui::Spring(1.0f, 0);

        ImGui::BeginVertical("out");
        for (u32 i = 0; i < graphData.Dependents.Count(); i++)
        {
            NodeEditor::BeginPin(++idCounter, NodeEditor::PinKind::Output);
            ImGui::Text(">");
            NodeEditor::EndPin();
        }
        ImGui::EndVertical();

        ImGui::EndHorizontal();
        NodeEditor::EndNode();

        // Set x position based on computed depth
        if (m_FirstRender)
        {
            ImVec2 size = ImGui::GetItemRectSize();
            ImVec2 curOffset = m_DepthOffsets[graphData.MaxDepth];
            float xOffset = 75.f;
            float yOffset = 50.f;
            float xPos = graphData.MaxDepth == 0 ? 0.f : m_DepthOffsets[graphData.MaxDepth - 1].x;

            NodeEditor::SetNodePosition((u64)plugin->GetUUID(), { xPos, curOffset.y });

            // Update offsets
            m_DepthOffsets[graphData.MaxDepth] = {
                std::max(xPos + size.x + xOffset, curOffset.x),
                curOffset.y + size.y + yOffset
            };
        }
    }
}
}
