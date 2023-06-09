#include "hepch.h"
#include "RenderPlugin.h"

#include "Heart/Core/App.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Renderer/SceneRenderer2.h"

namespace Heart
{
    void RenderPlugin::Render(const SceneRenderData& data)
    {
        if (App::Get().GetFrameCount() == m_LastRenderFrame)
            return;
        m_LastRenderFrame = App::Get().GetFrameCount();
        
        m_DependencyTasks.Clear();
        for (const auto& dep : m_Dependencies)
        {
            dep->Render(data);
            m_DependencyTasks.Add(dep->GetTask());
        }
        
        m_Task = TaskManager::Schedule(
            [this, data](){ RenderInternal(data); },
            Task::Priority::High,
            m_DependencyTasks
        );
    }

    void RenderPlugin::Resize()
    {
        for (const auto& dep : m_Dependencies)
            dep->Resize();

        ResizeInternal();
    }
}
