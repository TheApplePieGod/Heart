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
    SceneRenderer::SceneRenderer()
    {
        SubscribeToEmitter(&Window::GetMainWindow()); // We manually handle window resizes here

        m_RenderWidth = Window::GetMainWindow().GetWidth();
        m_RenderHeight = Window::GetMainWindow().GetHeight();

        Flourish::RenderGraphCreateInfo rgCreateInfo;
        rgCreateInfo.Usage = Flourish::RenderGraphUsageType::PerFrame;
        m_RenderGraph = Flourish::RenderGraph::Create(rgCreateInfo);

        CreateTextures();
        CreateDefaultResources();

        // Register plugins
        // TODO: parallel init? could do parallel init phase followed by gpunodebuilder
        // phase

        /*
        auto FrameData = RegisterPlugin<RenderPlugins::FrameData>("FrameData");
        auto LightingData = RegisterPlugin<RenderPlugins::LightingData>("LightingData");
        auto CBMESHCam = RegisterPlugin<RenderPlugins::ComputeMeshBatches>("CBMESHCam");
        auto EntityIds = RegisterPlugin<RenderPlugins::EntityIds>("EntityIds");
        auto TLAS = RegisterPlugin<RenderPlugins::TLAS>("TLAS");

        RenderPlugins::RenderMeshBatchesCreateInfo RBMESHCamCreateInfo;
        RBMESHCamCreateInfo.WriteNormals = true;
        RBMESHCamCreateInfo.FrameDataPluginName = FrameData->GetName();
        RBMESHCamCreateInfo.MeshBatchesPluginName = CBMESHCam->GetName();
        auto RBMESHCam = RegisterPlugin<RenderPlugins::RenderMeshBatches>("RBMESHCam", RBMESHCamCreateInfo);
        RBMESHCam->AddDependency(CBMESHCam->GetName(), GraphDependencyType::CPU);

        RenderPlugins::ComputeTextBatchesCreateInfo CBTEXTCamCreateInfo;
        auto CBTEXTCam = RegisterPlugin<RenderPlugins::ComputeTextBatches>("CBTEXTCam", CBTEXTCamCreateInfo);

        RenderPlugins::ComputeMaterialBatchesCreateInfo CBMATCamCreateInfo;
        CBMATCamCreateInfo.MeshBatchesPluginName = CBMESHCam->GetName();
        auto CBMATCam = RegisterPlugin<RenderPlugins::ComputeMaterialBatches>("CBMATCam", CBMATCamCreateInfo);
        CBMATCam->AddDependency(CBMESHCam->GetName(), GraphDependencyType::CPU);

        RenderPlugins::SSAOCreateInfo SSAOCreateInfo;
        SSAOCreateInfo.FrameDataPluginName = FrameData->GetName();
        SSAOCreateInfo.RenderMeshBatchesPluginName = RBMESHCam->GetName();
        auto SSAO = RegisterPlugin<RenderPlugins::SSAO>("SSAO", SSAOCreateInfo);
        SSAO->AddDependency(RBMESHCam->GetName(), GraphDependencyType::GPU);

        RenderPlugins::RenderEnvironmentMapCreateInfo ENVMAPCreateInfo;
        ENVMAPCreateInfo.FrameDataPluginName = FrameData->GetName();
        auto ENVMAP = RegisterPlugin<RenderPlugins::RenderEnvironmentMap>("ENVMAP", ENVMAPCreateInfo);

        RenderPlugins::TransparencyCompositeCreateInfo TransparencyCreateInfo;
        auto Transparency = RegisterPlugin<RenderPlugins::TransparencyComposite>("Transparency", TransparencyCreateInfo);

        RenderPlugins::RenderTextBatchesCreateInfo RBTEXTCamCreateInfo;
        RBTEXTCamCreateInfo.EntityIdsPluginName = EntityIds->GetName();
        RBTEXTCamCreateInfo.FrameDataPluginName = FrameData->GetName();
        RBTEXTCamCreateInfo.LightingDataPluginName = LightingData->GetName();
        RBTEXTCamCreateInfo.TextBatchesPluginName = CBTEXTCam->GetName();
        auto RBTEXTCam = RegisterPlugin<RenderPlugins::RenderTextBatches>("RBTEXTCam", RBTEXTCamCreateInfo);
        RBTEXTCam->AddDependency(CBTEXTCam->GetName(), GraphDependencyType::CPU);
        RBTEXTCam->AddDependency(ENVMAP->GetName(), GraphDependencyType::GPU);
        RBTEXTCam->AddDependency(RBMESHCam->GetName(), GraphDependencyType::GPU);

        RenderPlugins::RenderMaterialBatchesCreateInfo RBMATCamCreateInfo;
        // TODO: parameterize. will need to add support for specialization constants to do this
        RBMATCamCreateInfo.EntityIdsPluginName = EntityIds->GetName();
        RBMATCamCreateInfo.FrameDataPluginName = FrameData->GetName();
        RBMATCamCreateInfo.LightingDataPluginName = LightingData->GetName();
        RBMATCamCreateInfo.SSAOPluginName = SSAO->GetName();
        RBMATCamCreateInfo.MaterialBatchesPluginName = CBMATCam->GetName();
        RBMATCamCreateInfo.TransparencyCompositePluginName = Transparency->GetName();
        auto RBMATCam = RegisterPlugin<RenderPlugins::RenderMaterialBatches>("RBMATCam", RBMATCamCreateInfo);
        RBMATCam->AddDependency(CBMATCam->GetName(), GraphDependencyType::CPU);
        RBMATCam->AddDependency(SSAO->GetName(), GraphDependencyType::GPU);
        RBMATCam->AddDependency(RBTEXTCam->GetName(), GraphDependencyType::GPU);

        EntityIds->AddDependency(RBMATCam->GetName(), GraphDependencyType::GPU);

        RenderPlugins::InfiniteGridCreateInfo GRIDCreateInfo;
        GRIDCreateInfo.FrameDataPluginName = FrameData->GetName();
        GRIDCreateInfo.TransparencyCompositePluginName = Transparency->GetName();
        auto GRID = RegisterPlugin<RenderPlugins::InfiniteGrid>("GRID", GRIDCreateInfo);
        GRID->AddDependency(RBMATCam->GetName(), GraphDependencyType::GPU);

        Transparency->AddDependency(GRID->GetName(), GraphDependencyType::GPU);

        RenderPlugins::BloomCreateInfo BloomCreateInfo;
        auto Bloom = RegisterPlugin<RenderPlugins::Bloom>("Bloom", BloomCreateInfo);
        Bloom->AddDependency(Transparency->GetName(), GraphDependencyType::GPU);

        RenderPlugins::ColorGradingCreateInfo GRADINGCreateInfo;
        auto GRADING = RegisterPlugin<RenderPlugins::ColorGrading>("GRADING", GRADINGCreateInfo);
        GRADING->AddDependency(Bloom->GetName(), GraphDependencyType::GPU);
        */

        auto frameData = RegisterPlugin<RenderPlugins::FrameData>("FrameData");
        auto lightingData = RegisterPlugin<RenderPlugins::LightingData>("LightingData");
        auto entityIds = RegisterPlugin<RenderPlugins::EntityIds>("EntityIds");

        RenderPlugins::CollectMaterialsCreateInfo collectMatCreateInfo;
        auto collectMaterials = RegisterPlugin<RenderPlugins::CollectMaterials>("CollectMaterials", collectMatCreateInfo);

        RenderPlugins::ComputeMeshBatchesCreateInfo cbmeshcamCreateInfo;
        cbmeshcamCreateInfo.CollectMaterialsPluginName = collectMaterials->GetName();
        auto CBMESHCam = RegisterPlugin<RenderPlugins::ComputeMeshBatches>("CBMESHCam", cbmeshcamCreateInfo);
        CBMESHCam->AddDependency(collectMaterials->GetName(), GraphDependencyType::CPU);

        RenderPlugins::TLASCreateInfo tlasCreateInfo;
        tlasCreateInfo.CollectMaterialsPluginName = collectMaterials->GetName();
        auto tlas = RegisterPlugin<RenderPlugins::TLAS>("TLAS", tlasCreateInfo);
        tlas->AddDependency(collectMaterials->GetName(), GraphDependencyType::CPU);

        RenderPlugins::GBufferCreateInfo gBufferCreateInfo;
        gBufferCreateInfo.FrameDataPluginName = frameData->GetName();
        gBufferCreateInfo.MeshBatchesPluginName = CBMESHCam->GetName();
        gBufferCreateInfo.CollectMaterialsPluginName = collectMaterials->GetName();
        gBufferCreateInfo.EntityIdsPluginName = entityIds->GetName();
        auto gBuffer = RegisterPlugin<RenderPlugins::GBuffer>("GBuffer", gBufferCreateInfo);
        gBuffer->AddDependency(CBMESHCam->GetName(), GraphDependencyType::CPU);

        RenderPlugins::RayReflectionsCreateInfo rayReflCreateInfo;
        rayReflCreateInfo.FrameDataPluginName = frameData->GetName();
        rayReflCreateInfo.TLASPluginName = tlas->GetName();
        rayReflCreateInfo.LightingDataPluginName = lightingData->GetName();
        rayReflCreateInfo.CollectMaterialsPluginName = collectMaterials->GetName();
        rayReflCreateInfo.GBufferPluginName = gBuffer->GetName();
        auto rayReflections = RegisterPlugin<RenderPlugins::RayReflections>("RayReflections", rayReflCreateInfo);
        rayReflections->AddDependency(collectMaterials->GetName(), GraphDependencyType::CPU);
        rayReflections->AddDependency(tlas->GetName(), GraphDependencyType::CPU);
        rayReflections->AddDependency(tlas->GetName(), GraphDependencyType::GPU);
        rayReflections->AddDependency(gBuffer->GetName(), GraphDependencyType::GPU);

        // TODO: downscaled version
        RenderPlugins::SVGFCreateInfo svgfCreateInfo;
        svgfCreateInfo.InputPluginName = rayReflections->GetName();
        svgfCreateInfo.FrameDataPluginName = frameData->GetName();
        svgfCreateInfo.GBufferPluginName = gBuffer->GetName();
        auto svgf = RegisterPlugin<RenderPlugins::SVGF>("SVGF", svgfCreateInfo);
        svgf->AddDependency(rayReflections->GetName(), GraphDependencyType::GPU);

        RenderPlugins::RenderEnvironmentMapCreateInfo envMapCreateInfo;
        envMapCreateInfo.FrameDataPluginName = frameData->GetName();
        auto envMap = RegisterPlugin<RenderPlugins::RenderEnvironmentMap>("EnvMap", envMapCreateInfo);

        RenderPlugins::PBRCompositeCreateInfo pbrCompCreateInfo;
        pbrCompCreateInfo.ReflectionsInputPluginName = svgf->GetName();
        pbrCompCreateInfo.FrameDataPluginName = frameData->GetName();
        pbrCompCreateInfo.LightingDataPluginName = lightingData->GetName();
        pbrCompCreateInfo.GBufferPluginName = gBuffer->GetName();
        pbrCompCreateInfo.TLASPluginName = tlas->GetName();
        auto pbrComposite = RegisterPlugin<RenderPlugins::PBRComposite>("PBRComposite", pbrCompCreateInfo);
        pbrComposite->AddDependency(envMap->GetName(), GraphDependencyType::GPU);
        pbrComposite->AddDependency(svgf->GetName(), GraphDependencyType::GPU);
        pbrComposite->AddDependency(tlas->GetName(), GraphDependencyType::CPU);

        RenderPlugins::BloomCreateInfo BloomCreateInfo;
        auto bloom = RegisterPlugin<RenderPlugins::Bloom>("Bloom", BloomCreateInfo);
        bloom->AddDependency(pbrComposite->GetName(), GraphDependencyType::GPU);

        RenderPlugins::ColorGradingCreateInfo gradingCreateInfo;
        auto grading = RegisterPlugin<RenderPlugins::ColorGrading>("Grading", gradingCreateInfo);
        grading->AddDependency(bloom->GetName(), GraphDependencyType::GPU);

        RebuildGraph();
    }

    SceneRenderer::~SceneRenderer()
    {
        UnsubscribeFromEmitter(&Window::GetMainWindow());
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

        m_RenderGraph->Build();
    }

    TaskGroup SceneRenderer::Render(const SceneRenderData& data)
    {
        if (m_ShouldResize)
        {
            m_ShouldResize = false;
            Resize();
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
        CreateTextures();

        // Resize topologically in case of size dependencies
        for (const auto& leaf : m_CPUGraphData.Leaves)
            m_Plugins[leaf]->Resize();

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

    void SceneRenderer::CreateTextures()
    {
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_RenderWidth;
        texCreateInfo.Height = m_RenderHeight;
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics | Flourish::TextureUsageFlags::Compute;
        texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        m_RenderTexture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics;
        m_OutputTexture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.Format = Flourish::ColorFormat::Depth;
        m_DepthTexture = Flourish::Texture::Create(texCreateInfo);
    }

    void SceneRenderer::CreateDefaultResources()
    {
        Flourish::TextureCreateInfo envTexCreateInfo;
        envTexCreateInfo.Width = 256;
        envTexCreateInfo.Height = 256;
        envTexCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
        envTexCreateInfo.Usage = Flourish::TextureUsageFlags::Readonly;
        envTexCreateInfo.Writability = Flourish::TextureWritability::Once;
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
