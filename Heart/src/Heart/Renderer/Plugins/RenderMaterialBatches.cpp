#include "hepch.h"
#include "RenderMaterialBatches.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/LightingData.h"
#include "Heart/Renderer/Plugins/ComputeMaterialBatches.h"
#include "Heart/Renderer/SceneRenderer2.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Renderer/Material.h"
#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/DescriptorSet.h"

// REMOVE
#include "Flourish/Backends/Vulkan/DescriptorSet.h"

namespace Heart::RenderPlugins
{
    void RenderMaterialBatches::Initialize()
    {
        // Create default environment map cubemap object
        // TODO: this should be static somewhere (or an asset)
        {
            Flourish::TextureCreateInfo envTexCreateInfo;
            envTexCreateInfo.Width = 512;
            envTexCreateInfo.Height = 512;
            envTexCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
            envTexCreateInfo.Usage = Flourish::TextureUsageType::Readonly;
            envTexCreateInfo.Writability = Flourish::TextureWritability::Once;
            envTexCreateInfo.ArrayCount = 6;
            envTexCreateInfo.MipCount = 1;
            m_DefaultEnvironmentMap = Flourish::Texture::Create(envTexCreateInfo);
        }

        {
            Flourish::TextureCreateInfo texCreateInfo;
            texCreateInfo.Width = m_Info.Width;
            texCreateInfo.Height = m_Info.Height;
            texCreateInfo.ArrayCount = 1;
            texCreateInfo.MipCount = 1;
            texCreateInfo.Usage = Flourish::TextureUsageType::RenderTarget;
            texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
            texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
            texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
            m_RenderOutputTexture = Flourish::Texture::Create(texCreateInfo);
            texCreateInfo.Format = Flourish::ColorFormat::Depth;
            m_DepthTexture = Flourish::Texture::Create(texCreateInfo);
        }

        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::Four;
        rpCreateInfo.DepthAttachments.push_back({ Flourish::ColorFormat::Depth });
        rpCreateInfo.ColorAttachments.push_back({ m_RenderOutputTexture->GetColorFormat() });
        rpCreateInfo.Subpasses.push_back({
            {},
            { { Flourish::SubpassAttachmentType::Depth, 0 }, { Flourish::SubpassAttachmentType::Color, 0 } }
        });
        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_material_batches/Vertex.vert", true)->GetShader();
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_material_batches/Fragment.frag", true)->GetShader();
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
        pipelineCreateInfo.VertexLayout = Mesh::GetVertexLayout();
        pipelineCreateInfo.VertexInput = true;
        pipelineCreateInfo.BlendStates = { { false } };
        pipelineCreateInfo.DepthTest = true;
        pipelineCreateInfo.DepthWrite = true;
        pipelineCreateInfo.CullMode = Flourish::CullMode::Backface;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        //pipelineCreateInfo.CompatibleSubpasses = { 2 };
        auto pipeline = m_RenderPass->CreatePipeline("main", pipelineCreateInfo);

        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Info.Width;
        fbCreateInfo.Height = m_Info.Height;
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_RenderOutputTexture });
        fbCreateInfo.DepthAttachments.push_back({ m_DepthTexture });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.MaxEncoders = 1;
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::DescriptorSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::DescriptorSetWritability::PerFrame;
        m_DescriptorSet = pipeline->CreateDescriptorSet(0, dsCreateInfo);
    }

    void RenderMaterialBatches::Resize(u32 width, u32 height)
    {

    }

    void RenderMaterialBatches::RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::RenderMaterialBatches");

        auto frameDataPlugin = sceneRenderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto lightingDataPlugin = sceneRenderer->GetPlugin<RenderPlugins::LightingData>(m_Info.LightingDataPluginName);
        auto batchesPlugin = sceneRenderer->GetPlugin<RenderPlugins::ComputeMaterialBatches>(m_Info.MaterialBatchesPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto lightingDataBuffer = lightingDataPlugin->GetBuffer();
        const auto& batchData = batchesPlugin->GetBatchData();

        // TODO: this could probably be static
        m_DescriptorSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_DescriptorSet->BindBuffer(1, batchData.ObjectDataBuffer.get(), 0, batchData.ObjectDataBuffer->GetAllocatedCount());
        m_DescriptorSet->BindBuffer(2, batchData.MaterialDataBuffer.get(), 0, batchData.MaterialDataBuffer->GetAllocatedCount());
        m_DescriptorSet->BindBuffer(3, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        if (data.EnvMap)
        {
            m_DescriptorSet->BindTexture(5, data.EnvMap->GetIrradianceCubemap());
            m_DescriptorSet->BindTexture(6, data.EnvMap->GetPrefilterCubemap());
            m_DescriptorSet->BindTexture(7, data.EnvMap->GetBRDFTexture());
        }
        else
        {
            m_DescriptorSet->BindTexture(5, m_DefaultEnvironmentMap.get());
            m_DescriptorSet->BindTexture(6, m_DefaultEnvironmentMap.get());
            m_DescriptorSet->BindTextureLayer(7, m_DefaultEnvironmentMap.get(), 0, 0);
        }
        m_DescriptorSet->BindTextureLayer(8, m_DefaultEnvironmentMap.get(), 0, 0);
        m_DescriptorSet->FlushBindings();

        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
        encoder->BindPipeline("main");
        encoder->BindDescriptorSet(m_DescriptorSet.get(), 0);
        encoder->FlushDescriptorSet(0);

        // Initial (opaque) batches pass
        for (auto& batch : batchData.Batches)
        {
            //if (batch.Material->IsTranslucent())
            //    continue;

            encoder->BindDescriptorSet(batch.Material->GetDescriptorSet(), 1);
            encoder->FlushDescriptorSet(1);

            // Draw
            encoder->BindVertexBuffer(batch.Mesh->GetVertexBuffer());
            encoder->BindIndexBuffer(batch.Mesh->GetIndexBuffer());
            encoder->DrawIndexedIndirect(
                batchData.IndirectBuffer.get(), batch.First, batch.Count
            );
        }
        
        encoder->EndEncoding();
    }
}
