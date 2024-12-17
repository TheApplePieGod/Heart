#include "hepch.h"
#include "MobileSceneRenderer.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"
#include "Heart/Renderer/Plugins/AllPlugins.h"
#include "Flourish/Api/RenderContext.h"

namespace Heart
{
    void MobileSceneRenderer::RegisterPlugins()
    {
        //App::Get().GetWindow().GetRenderContext()->

        auto frameData = RegisterPlugin<RenderPlugins::FrameData>("FrameData");
        auto lightingData = RegisterPlugin<RenderPlugins::LightingData>("LightingData");

        RenderPlugins::CollectMaterialsCreateInfo collectMatCreateInfo;
        auto collectMaterials = RegisterPlugin<RenderPlugins::CollectMaterials>("CollectMaterials", collectMatCreateInfo);

        RenderPlugins::ClusteredLightingCreateInfo clusteredCreateInfo;
        clusteredCreateInfo.FrameDataPluginName = frameData->GetName();
        clusteredCreateInfo.LightingDataPluginName = lightingData->GetName();
        auto clusteredLighting = RegisterPlugin<RenderPlugins::ClusteredLighting>("ClusteredLighting", clusteredCreateInfo);

        RenderPlugins::ComputeMeshBatchesCreateInfo cbmeshcamCreateInfo;
        cbmeshcamCreateInfo.CollectMaterialsPluginName = collectMaterials->GetName();
        auto CBMESHCam = RegisterPlugin<RenderPlugins::ComputeMeshBatches>("CBMESHCam", cbmeshcamCreateInfo);
        CBMESHCam->AddDependency(collectMaterials->GetName(), GraphDependencyType::CPU);

        RenderPlugins::ComputeTextBatchesCreateInfo cbtextcamCreateInfo;
        cbtextcamCreateInfo.CollectMaterialsPluginName = collectMaterials->GetName();
        auto CBTEXTCam = RegisterPlugin<RenderPlugins::ComputeTextBatches>("CBTEXTCam", cbtextcamCreateInfo);
        CBTEXTCam->AddDependency(collectMaterials->GetName(), GraphDependencyType::CPU);

        RenderPlugins::RenderEnvironmentMapCreateInfo envMapCreateInfo;
        envMapCreateInfo.OutputTexture = m_OutputTexture;
        envMapCreateInfo.ClearOutput = true;
        envMapCreateInfo.FrameDataPluginName = frameData->GetName();
        auto envMap = RegisterPlugin<RenderPlugins::RenderEnvironmentMap>("EnvMap", envMapCreateInfo);

        RenderPlugins::ForwardCreateInfo forwardCreateInfo;
        forwardCreateInfo.OutputTexture = m_OutputTexture;
        forwardCreateInfo.DepthTexture = m_DepthTexture;
        forwardCreateInfo.FrameDataPluginName = frameData->GetName();
        forwardCreateInfo.LightingDataPluginName = lightingData->GetName();
        forwardCreateInfo.FrameDataPluginName = frameData->GetName();
        forwardCreateInfo.MeshBatchesPluginName = CBMESHCam->GetName();
        forwardCreateInfo.TextBatchesPluginName = CBTEXTCam->GetName();
        forwardCreateInfo.CollectMaterialsPluginName = collectMaterials->GetName();
        forwardCreateInfo.ClusteredLightingPluginName = clusteredLighting->GetName();
        auto forward = RegisterPlugin<RenderPlugins::Forward>("Forward", forwardCreateInfo);
        forward->AddDependency(CBMESHCam->GetName(), GraphDependencyType::CPU);
        forward->AddDependency(CBTEXTCam->GetName(), GraphDependencyType::CPU);
        forward->AddDependency(envMap->GetName(), GraphDependencyType::GPU);
        forward->AddDependency(clusteredLighting->GetName(), GraphDependencyType::CPU);
        forward->AddDependency(clusteredLighting->GetName(), GraphDependencyType::GPU);
        forward->AddInitDependency(clusteredLighting->GetName());

        RenderPlugins::ColorGradingCreateInfo gradingCreateInfo;
        gradingCreateInfo.InputTexture = m_OutputTexture;
        gradingCreateInfo.OutputTexture = m_OutputTexture;
        auto grading = RegisterPlugin<RenderPlugins::ColorGrading>("Grading", gradingCreateInfo);
        grading->AddDependency(forward->GetName(), GraphDependencyType::GPU);
    }

    void MobileSceneRenderer::CreateResources()
    {
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_RenderWidth;
        texCreateInfo.Height = m_RenderHeight;
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics | Flourish::TextureUsageFlags::Compute;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
        texCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
        Flourish::Texture::Replace(m_OutputTexture, texCreateInfo);
        texCreateInfo.Format = Flourish::ColorFormat::Depth;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics | Flourish::TextureUsageFlags::Transfer;
        Flourish::Texture::Replace(m_DepthTexture, texCreateInfo);
    }
}
