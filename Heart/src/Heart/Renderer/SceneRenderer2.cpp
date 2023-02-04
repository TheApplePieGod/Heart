#include "hepch.h"
#include "SceneRenderer2.h"

#include "Heart/Renderer/Plugins/AllPlugins.h"

namespace Heart
{
    SceneRenderer2::SceneRenderer2()
    {
        // Register plugins
        auto CBMESHCam = RegisterPlugin<RenderPlugins::ComputeMeshBatches>("CBMESHCam");
        
        m_PluginLeaves.Add(CBMESHCam->GetName());
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