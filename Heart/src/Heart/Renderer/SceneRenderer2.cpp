#include "hepch.h"
#include "SceneRenderer2.h"

#include "Heart/Renderer/Plugins/AllPlugins.h"
#include "Heart/Core/Window.h"
#include "Heart/Events/WindowEvents.h"
#include "Flourish/Api/Texture.h"

namespace Heart
{
    SceneRenderer2::SceneRenderer2()
    {
        SubscribeToEmitter(&Window::GetMainWindow()); // We manually handle window resizes here

        m_RenderWidth = Window::GetMainWindow().GetWidth();
        m_RenderHeight = Window::GetMainWindow().GetHeight();

        CreateTextures();

        // Register plugins

        auto FrameData = RegisterPlugin<RenderPlugins::FrameData>("FrameData");
        auto LightingData = RegisterPlugin<RenderPlugins::LightingData>("LightingData");
        auto CBMESHCam = RegisterPlugin<RenderPlugins::ComputeMeshBatches>("CBMESHCam");

        RenderPlugins::RenderMeshBatchesCreateInfo RBMESHCamCreateInfo;
        RBMESHCamCreateInfo.WriteNormals = true;
        RBMESHCamCreateInfo.FrameDataPluginName = FrameData->GetName();
        RBMESHCamCreateInfo.MeshBatchesPluginName = CBMESHCam->GetName();
        auto RBMESHCam = RegisterPlugin<RenderPlugins::RenderMeshBatches>("RBMESHCam", RBMESHCamCreateInfo);
        RBMESHCam->AddDependency(CBMESHCam);

        RenderPlugins::ComputeMaterialBatchesCreateInfo CBMATCamCreateInfo;
        CBMATCamCreateInfo.MeshBatchesPluginName = CBMESHCam->GetName();
        auto CBMATCam = RegisterPlugin<RenderPlugins::ComputeMaterialBatches>("CBMATCam", CBMATCamCreateInfo);
        CBMATCam->AddDependency(CBMESHCam);

        RenderPlugins::RenderMaterialBatchesCreateInfo RBMATCamCreateInfo;
        // TODO: parameterize. will need to add support for specialization constants to do this
        RBMATCamCreateInfo.CanOutputEntityIds = true; 
        RBMATCamCreateInfo.FrameDataPluginName = FrameData->GetName();
        RBMATCamCreateInfo.LightingDataPluginName = LightingData->GetName();
        RBMATCamCreateInfo.MaterialBatchesPluginName = CBMATCam->GetName();
        auto RBMATCam = RegisterPlugin<RenderPlugins::RenderMaterialBatches>("RBMATCam", RBMATCamCreateInfo);
        RBMATCam->AddDependency(CBMATCam);

        RenderPlugins::RenderEnvironmentMapCreateInfo ENVMAPCreateInfo;
        ENVMAPCreateInfo.FrameDataPluginName = FrameData->GetName();
        auto ENVMAP = RegisterPlugin<RenderPlugins::RenderEnvironmentMap>("ENVMAP", ENVMAPCreateInfo);
        
        RebuildGraph();
    }

    SceneRenderer2::~SceneRenderer2()
    {
        UnsubscribeFromEmitter(&Window::GetMainWindow());
    }

    void SceneRenderer2::RebuildGraph()
    {
        m_PluginLeaves.Clear();
        m_MaxDepth = 0;

        // Clear dependents
        for (const auto& pair : m_Plugins)
            pair.second->m_Dependents.Clear();

        // Populate dependents
        for (const auto& pair : m_Plugins)
            for (const auto& dep : pair.second->m_Dependencies)
                dep->m_Dependents.AddInPlace(pair.first);

        for (const auto& pair : m_Plugins)
        {
            // Populate leaves
            if (pair.second->m_Dependents.IsEmpty())
                m_PluginLeaves.AddInPlace(pair.second);

            // Run BFS from all roots to compute max depth
            if (pair.second->m_Dependencies.IsEmpty())
            {
                std::queue<std::pair<RenderPlugin*, u32>> searching;
                searching.emplace(pair.second.get(), 0);
                while (!searching.empty())
                {
                    auto pair = searching.front();
                    searching.pop();
                    for (const auto& dep : pair.first->m_Dependents)
                        searching.emplace(m_Plugins[dep].get(), pair.second + 1);
                    pair.first->m_MaxDepth = pair.second;
                    if (pair.second > m_MaxDepth)
                        m_MaxDepth = pair.second;
                }
            }
        }
    }

    TaskGroup SceneRenderer2::Render(const SceneRenderData& data)
    {
        if (m_ShouldResize)
        {
            m_ShouldResize = false;
            Resize();
        }

        TaskGroup group;
        for (const auto& leaf : m_PluginLeaves)
        {
            leaf->Render(data);
            group.AddTask(leaf->GetTask());
        }

        return group;
    }

    void SceneRenderer2::Resize()
    {
        CreateTextures();

        // Resize topologically in case of size dependencies
        for (const auto& leaf : m_PluginLeaves)
            leaf->Resize();
    }

    void SceneRenderer2::CreateTextures()
    {
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_RenderWidth;
        texCreateInfo.Height = m_RenderHeight;
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Usage = Flourish::TextureUsageType::RenderTarget;
        texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        m_RenderTexture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
        m_OutputTexture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.Format = Flourish::ColorFormat::Depth;
        m_DepthTexture = Flourish::Texture::Create(texCreateInfo);
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

}
