#include "hepch.h"
#include "SceneRenderer.h"

#include "Heart/Renderer/Plugins/AllPlugins.h"
#include "Heart/Core/Window.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Events/WindowEvents.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/RenderGraph.h"

namespace Heart
{
    SceneRenderer::SceneRenderer(bool debug)
        : m_Debug(debug)
    {
        SubscribeToEmitter(&Window::GetMainWindow()); // We manually handle window resizes here

        m_RenderWidth = Window::GetMainWindow().GetWidth();
        m_RenderHeight = Window::GetMainWindow().GetHeight();

        Flourish::RenderGraphCreateInfo rgCreateInfo;
        rgCreateInfo.Usage = Flourish::RenderGraphUsageType::PerFrame;
        m_RenderGraph = Flourish::RenderGraph::Create(rgCreateInfo);
    }

    SceneRenderer::~SceneRenderer()
    {
        UnsubscribeFromEmitter(&Window::GetMainWindow());
    }

    void SceneRenderer::Initialize()
    {
        CreateDefaultResources();
        CreateResources();
        RegisterPlugins();
        InitializeRegisteredPlugins();
        RebuildGraph();
    }

    void SceneRenderer::RebuildGraph()
    {
        RebuildGraphInternal(GraphDependencyType::CPU);
        RebuildGraphInternal(GraphDependencyType::GPU);

        m_RenderGraph->Clear();

        // Add all buffers as nodes
        for (const auto& pair : m_Plugins)
            if (pair.second->GetGraphNodeBuilder().GetNodeData().Buffer)
                pair.second->GetGraphNodeBuilder().AddToGraph(m_RenderGraph.get());

        // Define dependencies
        for (const auto& pair : m_Plugins)
            if (pair.second->GetCommandBuffer())
                for (const auto& dep : pair.second->GetGraphData(GraphDependencyType::GPU).Dependencies)
                    m_RenderGraph->AddExecutionDependency(pair.second->GetCommandBuffer(), m_Plugins[dep]->GetCommandBuffer());

        HE_ENGINE_LOG_DEBUG("Rebuilding SceneRenderer {}", (void*)this);
        for (const auto& pair : m_RenderGraph->GetNodes())
        {
            HE_ENGINE_LOG_DEBUG("SceneRenderer {}: Buffer {} has id {}", (void*)this, pair.second.Buffer->GetDebugName(), pair.second.Buffer->GetId());
        }

        m_RenderGraph->Build();
    }

    TaskGroup SceneRenderer::Render(const SceneRenderData& data)
    {
        if (!m_RenderGraph->IsBuilt())
        {
            Initialize();
            m_ShouldResize = false;
            m_ShouldRebuild = false;
        }

        if (m_ShouldResize)
        {
            m_ShouldResize = false;
            m_ShouldRebuild = false; // Resizing also rebuilds
            Resize();
        }

        if (m_ShouldRebuild)
        {
            m_ShouldRebuild = false;
            RebuildGraph();
        }

        TaskGroup group;
        for (const auto& leaf : m_CPUGraphData.Leaves)
        {
            auto& plugin = m_Plugins[leaf];
            plugin->Render(data);
            group.AddTask(plugin->GetTask());
        }

        return group;
    }

    SceneRenderer::GraphData& SceneRenderer::GetGraphData(GraphDependencyType depType)
    {
        switch (depType) {
            default: return m_CPUGraphData;
            case GraphDependencyType::CPU: return m_CPUGraphData;
            case GraphDependencyType::GPU: return m_GPUGraphData;
        }
    }

    void SceneRenderer::Resize()
    {
        CreateResources();

        TaskGroup group;
        for (const auto& pair : m_Plugins)
        {
            pair.second->Resize();
            group.AddTask(pair.second->GetTask());
        }

        group.Wait();

        RebuildGraph();
    }

    void SceneRenderer::RebuildGraphInternal(GraphDependencyType depType)
    {
        GraphData& graphData = GetGraphData(depType);
        graphData.Leaves.Clear();
        graphData.Roots.Clear();
        graphData.MaxDepth = 0;

        // Clear dependents
        for (const auto& pair : m_Plugins)
            pair.second->GetGraphData(depType).Dependents.Clear();

        // Populate dependents
        for (const auto& pair : m_Plugins)
            for (const auto& dep : pair.second->GetGraphData(depType).Dependencies)
                m_Plugins[dep]->GetGraphData(depType).Dependents.AddInPlace(pair.first);

        for (const auto& pair : m_Plugins)
        {
            // Populate leaves
            if (pair.second->GetGraphData(depType).Dependents.IsEmpty())
                graphData.Leaves.AddInPlace(pair.first);

            // Run BFS from all roots to compute max depth
            if (pair.second->GetGraphData(depType).Dependencies.empty())
            {
                graphData.Roots.AddInPlace(pair.first);

                std::queue<std::pair<RenderPlugin*, u32>> searching;
                searching.emplace(pair.second.get(), 0);
                while (!searching.empty())
                {
                    auto pair = searching.front();
                    searching.pop();
                    for (const auto& dep : pair.first->GetGraphData(depType).Dependents)
                        searching.emplace(m_Plugins[dep].get(), pair.second + 1);
                    if (pair.second > pair.first->GetGraphData(depType).MaxDepth)
                        pair.first->GetGraphData(depType).MaxDepth = pair.second;
                    if (pair.second > graphData.MaxDepth)
                        graphData.MaxDepth = pair.second;
                }
            }
        }
    }

    void SceneRenderer::InitializeRegisteredPlugins()
    {
        TaskGroup group;
        for (const auto& pair : m_Plugins)
        {
            pair.second->Initialize();
            group.AddTask(pair.second->GetTask());
        }

        group.Wait();
    }

    void SceneRenderer::CreateDefaultResources()
    {
        Flourish::TextureCreateInfo envTexCreateInfo;
        envTexCreateInfo.Width = 256;
        envTexCreateInfo.Height = 256;
        envTexCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
        envTexCreateInfo.Usage = Flourish::TextureUsageFlags::Readonly;
        envTexCreateInfo.ArrayCount = 6;
        envTexCreateInfo.MipCount = 1;
        m_DefaultEnvironmentMap = Flourish::Texture::Create(envTexCreateInfo);
    }

    void SceneRenderer::OnEvent(Event& event)
    {
        event.Map<WindowResizeEvent>(HE_BIND_EVENT_FN(SceneRenderer::OnWindowResize));
    }

    bool SceneRenderer::OnWindowResize(WindowResizeEvent& event)
    {
        if (event.GetWidth() == 0 || event.GetHeight() == 0)
            return false;

        m_RenderWidth = event.GetWidth();
        m_RenderHeight = event.GetHeight();
        m_ShouldResize = true;

        return false;
    }

}
