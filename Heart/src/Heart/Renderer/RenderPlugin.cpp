#include "hepch.h"
#include "RenderPlugin.h"

#include "Heart/Core/App.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Renderer/SceneRenderer.h"

namespace Heart
{
    void RenderPlugin::Initialize()
    {
        if (m_Initialized)
            return;
        m_Initialized = true;

        m_DependencyTasks.Clear();
        for (const auto& depName : m_InitDependencies)
        {
            auto dep = m_Renderer->GetPlugin(depName);
            dep->Initialize();
            m_DependencyTasks.Add(dep->GetTask());
        }
        
        m_Task = TaskManager::Schedule(
            [this](){ InitializeInternal(); },
            Task::Priority::High,
            m_DependencyTasks,
            m_Name
        );
    }

    void RenderPlugin::Render(const SceneRenderData& data)
    {
        if (App::Get().GetFrameCount() == m_LastRenderFrame)
            return;
        m_LastRenderFrame = App::Get().GetFrameCount();
        
        m_DependencyTasks.Clear();
        for (const auto& depName : m_CPUGraphData.Dependencies)
        {
            auto dep = m_Renderer->GetPlugin(depName);
            dep->Render(data);
            m_DependencyTasks.Add(dep->GetTask());
        }
        
        m_Task = TaskManager::Schedule(
            [this, data](){ RenderInternal(data); },
            Task::Priority::High,
            m_DependencyTasks,
            m_Name
        );
    }

    void RenderPlugin::Resize()
    {
        if (App::Get().GetFrameCount() == m_LastResizeFrame)
            return;
        m_LastResizeFrame = App::Get().GetFrameCount();

        m_DependencyTasks.Clear();
        for (const auto& depName : m_InitDependencies)
        {
            auto dep = m_Renderer->GetPlugin(depName);
            dep->Resize();
            m_DependencyTasks.Add(dep->GetTask());
        }
        
        m_Task = TaskManager::Schedule(
            [this](){ ResizeInternal(); },
            Task::Priority::High,
            m_DependencyTasks,
            m_Name
        );
    }

    void RenderPlugin::AddDependency(const HString8& name, GraphDependencyType depType)
    {
        GetGraphData(depType).Dependencies.insert(name);
    }

    void RenderPlugin::AddInitDependency(const HString8& name)
    {
        m_InitDependencies.insert(name);
    }

    RenderPlugin::GraphData& RenderPlugin::GetGraphData(GraphDependencyType depType)
    {
        switch (depType) {
            case GraphDependencyType::CPU: return m_CPUGraphData;
            case GraphDependencyType::GPU: return m_GPUGraphData;
        }
    }
}
