#include "hepch.h"
#include "DesktopSceneRenderer.h"

#include "Heart/Renderer/Plugins/AllPlugins.h"

namespace Heart
{
    void DesktopSceneRenderer::RegisterPlugins()
    {
        // TODO: transparency broken (again)

        bool rayTracing = Flourish::Context::FeatureTable().RayTracing;

        auto frameData = RegisterPlugin<RenderPlugins::FrameData>("FrameData");
        auto lightingData = RegisterPlugin<RenderPlugins::LightingData>("LightingData");
        auto entityIds = RegisterPlugin<RenderPlugins::EntityIds>("EntityIds");

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

        RenderPlugins::GBufferCreateInfo gBufferCreateInfo;
        gBufferCreateInfo.KeepHistory = rayTracing;
        gBufferCreateInfo.MipCount = rayTracing ? 2 : 1;
        gBufferCreateInfo.FrameDataPluginName = frameData->GetName();
        gBufferCreateInfo.MeshBatchesPluginName = CBMESHCam->GetName();
        gBufferCreateInfo.TextBatchesPluginName = CBTEXTCam->GetName();
        gBufferCreateInfo.CollectMaterialsPluginName = collectMaterials->GetName();
        gBufferCreateInfo.EntityIdsPluginName = entityIds->GetName();
        auto gBuffer = RegisterPlugin<RenderPlugins::GBuffer>("GBuffer", gBufferCreateInfo);
        gBuffer->AddDependency(CBMESHCam->GetName(), GraphDependencyType::CPU);
        gBuffer->AddDependency(CBTEXTCam->GetName(), GraphDependencyType::CPU);
        gBuffer->AddInitDependency(entityIds->GetName());

        RenderPlugins::SSAOCreateInfo ssaoCreateInfo;
        ssaoCreateInfo.FrameDataPluginName = frameData->GetName();
        ssaoCreateInfo.InputDepthTexture = gBuffer->GetGBufferDepth();
        ssaoCreateInfo.InputNormalsTexture = gBuffer->GetGBuffer2();
        auto ssao = RegisterPlugin<RenderPlugins::SSAO>("SSAO", ssaoCreateInfo);
        ssao->AddDependency(gBuffer->GetName(), GraphDependencyType::CPU);
        ssao->AddDependency(gBuffer->GetName(), GraphDependencyType::GPU);

        RenderPlugins::RenderEnvironmentMapCreateInfo envMapCreateInfo;
        envMapCreateInfo.OutputTexture = m_OutputTexture;
        envMapCreateInfo.ClearOutput = true;
        envMapCreateInfo.FrameDataPluginName = frameData->GetName();
        auto envMap = RegisterPlugin<RenderPlugins::RenderEnvironmentMap>("EnvMap", envMapCreateInfo);

        HString8 pbrCompositeName;
        if (rayTracing)
        {
            RenderPlugins::TLASCreateInfo tlasCreateInfo;
            tlasCreateInfo.CollectMaterialsPluginName = collectMaterials->GetName();
            auto tlas = RegisterPlugin<RenderPlugins::TLAS>("TLAS", tlasCreateInfo);
            tlas->AddDependency(collectMaterials->GetName(), GraphDependencyType::CPU);

            RenderPlugins::RayReflectionsCreateInfo rayReflCreateInfo;
            rayReflCreateInfo.OutputTexture = m_RayReflectionsTexture;
            rayReflCreateInfo.FrameDataPluginName = frameData->GetName();
            rayReflCreateInfo.TLASPluginName = tlas->GetName();
            rayReflCreateInfo.LightingDataPluginName = lightingData->GetName();
            rayReflCreateInfo.CollectMaterialsPluginName = collectMaterials->GetName();
            rayReflCreateInfo.GBufferPluginName = gBuffer->GetName();
            auto rayReflections = RegisterPlugin<RenderPlugins::RayReflections>("RayReflections", rayReflCreateInfo);
            rayReflections->AddDependency(collectMaterials->GetName(), GraphDependencyType::CPU);
            rayReflections->AddDependency(tlas->GetName(), GraphDependencyType::CPU);
            rayReflections->AddDependency(tlas->GetName(), GraphDependencyType::GPU);
            rayReflections->AddDependency(lightingData->GetName(), GraphDependencyType::CPU);
            rayReflections->AddDependency(lightingData->GetName(), GraphDependencyType::GPU);
            rayReflections->AddDependency(gBuffer->GetName(), GraphDependencyType::GPU);
            rayReflections->AddInitDependency(gBuffer->GetName());

            // TODO: downscaled version
            RenderPlugins::SVGFCreateInfo svgfCreateInfo;
            svgfCreateInfo.InputTexture = m_RayReflectionsTexture;
            svgfCreateInfo.OutputTexture = m_RayReflectionsTexture;
            svgfCreateInfo.FrameDataPluginName = frameData->GetName();
            svgfCreateInfo.GBufferPluginName = gBuffer->GetName();
            auto svgf = RegisterPlugin<RenderPlugins::SVGF>("SVGF", svgfCreateInfo);
            svgf->AddDependency(rayReflections->GetName(), GraphDependencyType::GPU);
            svgf->AddInitDependency(rayReflections->GetName());
            svgf->AddInitDependency(gBuffer->GetName());

            RenderPlugins::RayPBRCompositeCreateInfo pbrCompCreateInfo;
            pbrCompCreateInfo.ReflectionsInputTexture = m_RayReflectionsTexture;
            pbrCompCreateInfo.OutputTexture = m_OutputTexture;
            pbrCompCreateInfo.FrameDataPluginName = frameData->GetName();
            pbrCompCreateInfo.LightingDataPluginName = lightingData->GetName();
            pbrCompCreateInfo.GBufferPluginName = gBuffer->GetName();
            pbrCompCreateInfo.ClusteredLightingPluginName = clusteredLighting->GetName();
            pbrCompCreateInfo.TLASPluginName = tlas->GetName();
            pbrCompCreateInfo.CollectMaterialsPluginName = collectMaterials->GetName();
            auto pbrComposite = RegisterPlugin<RenderPlugins::RayPBRComposite>("RayPBRComposite", pbrCompCreateInfo);
            pbrComposite->AddDependency(envMap->GetName(), GraphDependencyType::GPU);
            pbrComposite->AddDependency(svgf->GetName(), GraphDependencyType::GPU);
            pbrComposite->AddDependency(ssao->GetName(), GraphDependencyType::GPU);
            pbrComposite->AddDependency(tlas->GetName(), GraphDependencyType::CPU);
            pbrComposite->AddDependency(clusteredLighting->GetName(), GraphDependencyType::CPU);
            pbrComposite->AddDependency(clusteredLighting->GetName(), GraphDependencyType::GPU);
            pbrComposite->AddInitDependency(svgf->GetName());
            pbrComposite->AddInitDependency(gBuffer->GetName());
            pbrComposite->AddInitDependency(clusteredLighting->GetName());

            pbrCompositeName = pbrComposite->GetName();
        }
        else
        {
            RenderPlugins::PBRCompositeCreateInfo pbrCompCreateInfo;
            pbrCompCreateInfo.OutputTexture = m_OutputTexture;
            pbrCompCreateInfo.FrameDataPluginName = frameData->GetName();
            pbrCompCreateInfo.LightingDataPluginName = lightingData->GetName();
            pbrCompCreateInfo.GBufferPluginName = gBuffer->GetName();
            pbrCompCreateInfo.SSAOPluginName = ssao->GetName();
            pbrCompCreateInfo.ClusteredLightingPluginName = clusteredLighting->GetName();
            auto pbrComposite = RegisterPlugin<RenderPlugins::PBRComposite>("PBRComposite", pbrCompCreateInfo);
            pbrComposite->AddDependency(ssao->GetName(), GraphDependencyType::GPU);
            pbrComposite->AddDependency(envMap->GetName(), GraphDependencyType::GPU);
            pbrComposite->AddDependency(clusteredLighting->GetName(), GraphDependencyType::CPU);
            pbrComposite->AddDependency(clusteredLighting->GetName(), GraphDependencyType::GPU);
            pbrComposite->AddInitDependency(gBuffer->GetName());
            pbrComposite->AddInitDependency(ssao->GetName());
            pbrComposite->AddInitDependency(clusteredLighting->GetName());

            pbrCompositeName = pbrComposite->GetName();
        }

        RenderPlugins::BlitTextureCreateInfo blitCreateInfo;
        blitCreateInfo.SrcDynamicLayerIndex = true;
        blitCreateInfo.SrcLayerIndex = rayTracing ? 2 : 1;
        blitCreateInfo.SrcTexture = gBuffer->GetGBufferDepth();
        blitCreateInfo.DstTexture = m_DepthTexture;
        auto blit = RegisterPlugin<RenderPlugins::BlitTexture>("GBuf Depth Blit", blitCreateInfo);
        blit->AddDependency(gBuffer->GetName(), GraphDependencyType::CPU);
        blit->AddDependency(pbrCompositeName, GraphDependencyType::GPU);

        RenderPlugins::SplatCreateInfo splatCreateInfo;
        splatCreateInfo.OutputColorTexture = m_OutputTexture;
        splatCreateInfo.OutputDepthTexture = m_DepthTexture;
        splatCreateInfo.ClearColorOutput = false;
        splatCreateInfo.ClearDepthOutput = false;
        splatCreateInfo.FrameDataPluginName = frameData->GetName();
        auto splat = RegisterPlugin<RenderPlugins::Splat>("Splat", splatCreateInfo);
        splat->AddDependency(blit->GetName(), GraphDependencyType::GPU);

        RenderPlugins::InfiniteGridCreateInfo gridCreateInfo;
        gridCreateInfo.OutputColorTexture = m_OutputTexture;
        gridCreateInfo.OutputDepthTexture = m_DepthTexture;
        gridCreateInfo.ClearColorOutput = false;
        gridCreateInfo.ClearDepthOutput = false;
        gridCreateInfo.FrameDataPluginName = frameData->GetName();
        auto grid = RegisterPlugin<RenderPlugins::InfiniteGrid>("Grid", gridCreateInfo);
        grid->AddDependency(splat->GetName(), GraphDependencyType::GPU);

        RenderPlugins::BloomCreateInfo bloomCreateInfo;
        bloomCreateInfo.InputTexture = m_OutputTexture;
        bloomCreateInfo.OutputTexture = m_OutputTexture;
        auto bloom = RegisterPlugin<RenderPlugins::Bloom>("Bloom", bloomCreateInfo);
        bloom->AddDependency(grid->GetName(), GraphDependencyType::GPU);

        RenderPlugins::ColorGradingCreateInfo gradingCreateInfo;
        gradingCreateInfo.InputTexture = m_OutputTexture;
        gradingCreateInfo.OutputTexture = m_OutputTexture;
        auto grading = RegisterPlugin<RenderPlugins::ColorGrading>("Grading", gradingCreateInfo);
        grading->AddDependency(bloom->GetName(), GraphDependencyType::GPU);
    }

    void DesktopSceneRenderer::CreateResources()
    {
        bool rayTracing = Flourish::Context::FeatureTable().RayTracing;

        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_RenderWidth;
        texCreateInfo.Height = m_RenderHeight;
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics | Flourish::TextureUsageFlags::Compute;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        Flourish::Texture::Replace(m_OutputTexture, texCreateInfo);
        texCreateInfo.Format = Flourish::ColorFormat::Depth;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics | Flourish::TextureUsageFlags::Transfer;
        Flourish::Texture::Replace(m_DepthTexture, texCreateInfo);

        if (rayTracing)
        {
            texCreateInfo.Width = m_RenderWidth / 2;
            texCreateInfo.Height = m_RenderHeight / 2;
            texCreateInfo.ArrayCount = 1;
            texCreateInfo.MipCount = 1;
            texCreateInfo.Usage = Flourish::TextureUsageFlags::Compute;
            texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
            m_RayReflectionsTexture = Flourish::Texture::Create(texCreateInfo);
        }
    }
}
