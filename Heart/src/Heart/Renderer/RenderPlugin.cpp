#include "hepch.h"
#include "RenderPlugin.h"

#include "Heart/Core/App.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Renderer/SceneRenderer2.h"

namespace Heart
{
    void RenderPlugin::Render(const SceneRenderData& data, SceneRenderer2* sceneRenderer)
    {
        if (App::Get().GetFrameCount() == m_LastRenderFrame)
            return;
        m_LastRenderFrame = App::Get().GetFrameCount();
        
        m_DependencyTasks.Clear();
        for (const auto& dep : m_Dependencies)
        {
            dep->Render(data, sceneRenderer);
            m_DependencyTasks.Add(dep->GetTask());
        }
        
        m_Task = TaskManager::Schedule(
            [this, data, sceneRenderer](){ RenderInternal(data, sceneRenderer); },
            Task::Priority::High,
            m_DependencyTasks
        );
    }
}