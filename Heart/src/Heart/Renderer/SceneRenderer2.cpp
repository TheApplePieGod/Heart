#include "hepch.h"
#include "SceneRenderer2.h"

#include "Heart/Renderer/Plugins/AllPlugins.h"
#include "Heart/Core/Window.h"
#include "Heart/Events/WindowEvents.h"

namespace Heart
{
    SceneRenderer2::SceneRenderer2()
    {
        SubscribeToEmitter(&Window::GetMainWindow()); // We manually handle window resizes here

        m_RenderWidth = Window::GetMainWindow().GetWidth();
        m_RenderHeight = Window::GetMainWindow().GetHeight();

        // Register plugins

        auto FrameData = RegisterPlugin<RenderPlugins::FrameData>("FrameData");
        auto CBMESHCam = RegisterPlugin<RenderPlugins::ComputeMeshBatches>("CBMESHCam");

        RenderPlugins::RenderMeshBatchesCreateInfo RBMESHCamCreateInfo;
        RBMESHCamCreateInfo.Width = m_RenderWidth;
        RBMESHCamCreateInfo.Height = m_RenderHeight;
        RBMESHCamCreateInfo.WriteNormals = true;
        RBMESHCamCreateInfo.FrameDataPluginName = FrameData->GetName();
        RBMESHCamCreateInfo.MeshBatchesPluginName = CBMESHCam->GetName();
        auto RBMESHCam = RegisterPlugin<RenderPlugins::RenderMeshBatches>("RBMESHCam", RBMESHCamCreateInfo);
        RBMESHCam->AddDependency(FrameData);
        RBMESHCam->AddDependency(CBMESHCam);
        
        m_PluginLeaves.Add(RBMESHCam->GetName());
    }

    SceneRenderer2::~SceneRenderer2()
    {
        UnsubscribeFromEmitter(&Window::GetMainWindow());
    }

    void SceneRenderer2::OnEvent(Event& event)
    {
        event.Map<WindowResizeEvent>(HE_BIND_EVENT_FN(SceneRenderer2::OnWindowResize));
    }

    bool SceneRenderer2::OnWindowResize(WindowResizeEvent& event)
    {
        if (event.GetWidth() == 0 || event.GetHeight() == 0)
            return false;

        m_RenderWidth = event.GetWidth();
        m_RenderHeight = event.GetHeight();
        m_ShouldResize = true;

        return false;
    }

    TaskGroup SceneRenderer2::Render(const SceneRenderData& data)
    {
        TaskGroup group;
        for (const auto& leaf : m_PluginLeaves)
        {
            auto& plugin = m_Plugins[leaf];
            plugin->Render(data, this);
            group.AddTask(plugin->GetTask());
        }

        return group;
    }
}
