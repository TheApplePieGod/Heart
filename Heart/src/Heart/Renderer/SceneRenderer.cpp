#include "hepch.h"
#include "SceneRenderer.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"
#include "Heart/Core/Timing.h"
#include "Heart/Task/TaskManager.h"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/GraphicsPipeline.h"
#include "Flourish/Api/ComputePipeline.h"
#include "Flourish/Api/RenderContext.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/Buffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/TransferCommandEncoder.h"
#include "Heart/Renderer/Material.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Core/Camera.h"
#include "Heart/Events/WindowEvents.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/RenderScene.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Asset/FontAsset.h"
#include "glm/gtc/matrix_transform.hpp"

namespace Heart
{
    SceneRenderer::SceneRenderer()
    {
        SubscribeToEmitter(&Window::GetMainWindow()); // We manually handle window resizes here

        m_RenderWidth = Window::GetMainWindow().GetWidth();
        m_RenderHeight = Window::GetMainWindow().GetHeight();

        Initialize();
    }

    SceneRenderer::~SceneRenderer()
    {
        UnsubscribeFromEmitter(&Window::GetMainWindow());
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

    void SceneRenderer::Initialize()
    {
        // Create default environment map cubemap object
        // TODO: we should be reusing this elsewhere
        Flourish::TextureCreateInfo envTexCreateInfo;
        envTexCreateInfo.Width = 512;
        envTexCreateInfo.Height = 512;
        envTexCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
        envTexCreateInfo.Usage = Flourish::TextureUsageType::Readonly;
        envTexCreateInfo.ArrayCount = 6;
        envTexCreateInfo.MipCount = 5;
        m_DefaultEnvironmentMap = Flourish::Texture::Create(envTexCreateInfo);

        CreateBuffers();
        CreateTextures();
        CreateRenderPasses();
        CreateFramebuffers();
        CreateComputeObjects();
        InitializeGridBuffers();
        InitializeSSAOData();

        // Create command buffers
        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.MaxEncoders = 5;
        m_MainCommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        cbCreateInfo.MaxEncoders = m_MaxBloomMipCount * 2; 
        m_BloomCommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        cbCreateInfo.MaxEncoders = 1; 
        m_FinalCommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        cbCreateInfo.MaxEncoders = 1; 
        m_SSAOCommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);
    }

    void SceneRenderer::Resize()
    {
        m_ShouldResize = false;

        CreateTextures();
        CreateFramebuffers();
        
        if (m_PhysicsDebugRenderer)
            m_PhysicsDebugRenderer->Resize(m_RenderWidth, m_RenderHeight);
    }

    void SceneRenderer::CreateBuffers()
    {
        u32 maxObjects = 10000;
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Uniform;
        bufCreateInfo.Stride = sizeof(FrameData);
        bufCreateInfo.ElementCount = 1;
        m_FrameDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Stride = sizeof(BloomData);
        bufCreateInfo.ElementCount = m_MaxBloomMipCount * 2;
        m_BloomDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

        bufCreateInfo.Type = Flourish::BufferType::Uniform;
        bufCreateInfo.Stride = sizeof(SSAOData);
        bufCreateInfo.ElementCount = 1;
        m_SSAODataBuffer = Flourish::Buffer::Create(bufCreateInfo);

        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Stride = sizeof(LightData);
        bufCreateInfo.ElementCount = 500;
        m_LightingDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

        for (u32 i = 0; i < Flourish::Context::FrameBufferCount(); i++)
        {
            bufCreateInfo.Usage = Flourish::BufferUsageType::DynamicOneFrame;
            bufCreateInfo.Type = Flourish::BufferType::Storage;
            bufCreateInfo.Stride = sizeof(ObjectData);
            bufCreateInfo.ElementCount = maxObjects;
            m_BatchRenderData[i].ObjectDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

            bufCreateInfo.Usage = Flourish::BufferUsageType::DynamicOneFrame;
            bufCreateInfo.Type = Flourish::BufferType::Storage;
            bufCreateInfo.Stride = sizeof(MaterialData);
            bufCreateInfo.ElementCount = maxObjects;
            m_BatchRenderData[i].MaterialDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

            bufCreateInfo.Usage = Flourish::BufferUsageType::DynamicOneFrame;
            bufCreateInfo.Type = Flourish::BufferType::Indirect;
            bufCreateInfo.Stride = sizeof(IndexedIndirectCommand);
            bufCreateInfo.ElementCount = maxObjects;
            m_BatchRenderData[i].IndirectBuffer = Flourish::Buffer::Create(bufCreateInfo);
        }
    }

    void SceneRenderer::CreateTextures()
    {
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_RenderWidth;
        texCreateInfo.Height = m_RenderHeight;
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        texCreateInfo.Usage = Flourish::TextureUsageType::RenderTarget;
        texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
        m_RenderOutputTexture = Flourish::Texture::Create(texCreateInfo);
        
        texCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
        texCreateInfo.Usage = Flourish::TextureUsageType::ComputeTarget;
        texCreateInfo.MipCount = 1;
        m_FinalTexture = Flourish::Texture::Create(texCreateInfo);

        texCreateInfo.Format = Flourish::ColorFormat::R32_FLOAT;
        texCreateInfo.Usage = Flourish::TextureUsageType::RenderTarget;
        texCreateInfo.MipCount = 1;
        m_EntityIdsTexture = Flourish::Texture::Create(texCreateInfo);

        texCreateInfo.Format = Flourish::ColorFormat::Depth;
        texCreateInfo.Usage = Flourish::TextureUsageType::RenderTarget;
        texCreateInfo.MipCount = 1;
        m_DepthTexture = Flourish::Texture::Create(texCreateInfo);

        texCreateInfo.Format = Flourish::ColorFormat::R16_FLOAT;
        texCreateInfo.Usage = Flourish::TextureUsageType::ComputeTarget;
        texCreateInfo.MipCount = 1;
        m_SSAOTexture = Flourish::Texture::Create(texCreateInfo);

        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        texCreateInfo.Usage = Flourish::TextureUsageType::ComputeTarget;
        texCreateInfo.MipCount = 7;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge };
        m_BloomDownsampleBufferTexture = Flourish::Texture::Create(texCreateInfo);
        m_BloomUpsampleBufferTexture = Flourish::Texture::Create(texCreateInfo);
        
        m_BloomMipCount = m_BloomDownsampleBufferTexture->GetMipCount();
    }

    void SceneRenderer::CreateRenderPasses()
    {
        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::Four;
        rpCreateInfo.ColorAttachments.push_back({ m_EntityIdsTexture->GetColorFormat() });    // Entity ids           [0]
        rpCreateInfo.ColorAttachments.push_back({ Flourish::ColorFormat::RGBA16_FLOAT });     // Transparency data    [1]
        rpCreateInfo.ColorAttachments.push_back({ Flourish::ColorFormat::R16_FLOAT });        // Transparency data    [2]
        rpCreateInfo.ColorAttachments.push_back({ m_RenderOutputTexture->GetColorFormat() }); // Output target        [3]
        rpCreateInfo.DepthAttachments.push_back({ Flourish::ColorFormat::Depth });
        rpCreateInfo.Subpasses.push_back({ // Environment map
            {},
            { { Flourish::SubpassAttachmentType::Color, 3 } }
        });
        rpCreateInfo.Subpasses.push_back({ // Grid
            {},
            { { Flourish::SubpassAttachmentType::Color, 3 } }
        });
        rpCreateInfo.Subpasses.push_back({ // Opaque
            {},
            { { Flourish::SubpassAttachmentType::Depth, 0 }, { Flourish::SubpassAttachmentType::Color, 3 }, { Flourish::SubpassAttachmentType::Color, 0 } } 
        });
        rpCreateInfo.Subpasses.push_back({ // Transparent color
            {},
            { { Flourish::SubpassAttachmentType::Depth, 0 }, { Flourish::SubpassAttachmentType::Color, 0 }, { Flourish::SubpassAttachmentType::Color, 1 }, { Flourish::SubpassAttachmentType::Color, 2 } } 
        });
        rpCreateInfo.Subpasses.push_back({ // Composite
            { { Flourish::SubpassAttachmentType::Color, 1 }, { Flourish::SubpassAttachmentType::Color, 2 } },
            { { Flourish::SubpassAttachmentType::Depth, 0 }, { Flourish::SubpassAttachmentType::Color, 3 } } 
        });
        m_MainRenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/Grid.vert", true)->GetShader();
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/Grid.frag", true)->GetShader();
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::LineList;
        pipelineCreateInfo.VertexLayout = { Flourish::BufferDataType::Float3 };
        pipelineCreateInfo.VertexInput = true;
        pipelineCreateInfo.BlendStates = { { false } };
        /*
        pipelineCreateInfo.DepthTest = false;
        pipelineCreateInfo.DepthWrite = false;
        */
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        pipelineCreateInfo.CompatibleSubpasses = { 1 };
        m_MainRenderPass->CreatePipeline("grid", pipelineCreateInfo);

        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/Skybox.vert", true)->GetShader();
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/Skybox.frag", true)->GetShader();
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
        pipelineCreateInfo.VertexLayout = Heart::Mesh::GetVertexLayout();
        pipelineCreateInfo.BlendStates = { { false } };
        /*
        pipelineCreateInfo.DepthTest = false;
        pipelineCreateInfo.DepthWrite = false;
        */
        pipelineCreateInfo.CullMode = Flourish::CullMode::Frontface;
        pipelineCreateInfo.CompatibleSubpasses = { 0 };
        m_MainRenderPass->CreatePipeline("skybox", pipelineCreateInfo);

        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/PBR.vert", true)->GetShader();
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/PBR.frag", true)->GetShader();
        pipelineCreateInfo.BlendStates = { { false }, { false } };
        /*
        pipelineCreateInfo.DepthTest = true;
        pipelineCreateInfo.DepthWrite = true;
        */
        pipelineCreateInfo.CullMode = Flourish::CullMode::Backface;
        pipelineCreateInfo.CompatibleSubpasses = { 2 };
        m_MainRenderPass->CreatePipeline("pbr", pipelineCreateInfo);

        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/PBR.vert", true)->GetShader();
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/TextPBR.frag", true)->GetShader();
        pipelineCreateInfo.BlendStates = { { false }, { false } };
        /*
        pipelineCreateInfo.DepthTest = true;
        pipelineCreateInfo.DepthWrite = true;
        */
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        pipelineCreateInfo.CompatibleSubpasses = { 2 };
        m_MainRenderPass->CreatePipeline("pbrText", pipelineCreateInfo);

        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/PBRTransparentColor.frag", true)->GetShader();
        pipelineCreateInfo.BlendStates = {
            { false },
            { true, Flourish::BlendFactor::One, Flourish::BlendFactor::One, Flourish::BlendFactor::One, Flourish::BlendFactor::One },
            { true, Flourish::BlendFactor::Zero, Flourish::BlendFactor::OneMinusSrcColor, Flourish::BlendFactor::Zero, Flourish::BlendFactor::OneMinusSrcAlpha }
        };
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        pipelineCreateInfo.CompatibleSubpasses = { 3 };
        m_MainRenderPass->CreatePipeline("pbrTpColor", pipelineCreateInfo);

        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/FullscreenTriangle.vert", true)->GetShader();
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/TransparentComposite.frag", true)->GetShader();
        pipelineCreateInfo.VertexInput = false;
        pipelineCreateInfo.BlendStates = {
            { true, Flourish::BlendFactor::OneMinusSrcAlpha, Flourish::BlendFactor::SrcAlpha, Flourish::BlendFactor::SrcAlpha, Flourish::BlendFactor::OneMinusSrcAlpha }
        };
        //pipelineCreateInfo.DepthWrite = false;
        pipelineCreateInfo.CompatibleSubpasses = { 4 };
        m_MainRenderPass->CreatePipeline("tpComposite", pipelineCreateInfo);
    }

    void SceneRenderer::CreateFramebuffers()
    {
        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_MainRenderPass;
        fbCreateInfo.Width = m_RenderWidth;
        fbCreateInfo.Height = m_RenderHeight;
        fbCreateInfo.ColorAttachments.push_back({ { -1.f, 0.f, 0.f, 0.f }, m_EntityIdsTexture });   // Entity ids           [0]
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f } });                        // Transparency data    [1]
        fbCreateInfo.ColorAttachments.push_back({ { 1.f, 0.f, 0.f, 0.f } });                        // Transparency data    [2]
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_RenderOutputTexture }); // Output target        [3]
        fbCreateInfo.DepthAttachments.push_back({ m_DepthTexture });
        m_MainFramebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Pixel;
        bufCreateInfo.Stride = sizeof(float);
        bufCreateInfo.ElementCount = m_RenderWidth * m_RenderHeight;
        m_EntityIdsPixelBuffer = Flourish::Buffer::Create(bufCreateInfo);
    }

    void SceneRenderer::CreateComputeObjects()
    {
        Flourish::ComputePipelineCreateInfo compCreateInfo;
        compCreateInfo.ComputeShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/ComputeBloomDownsample.comp", true)->GetShader();
        m_BloomDownsampleComputePipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        compCreateInfo.ComputeShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/ComputeBloomUpsample.comp", true)->GetShader();
        m_BloomUpsampleComputePipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        compCreateInfo.ComputeShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/FinalComposite.comp", true)->GetShader();
        m_FinalCompositeComputePipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        compCreateInfo.ComputeShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/SSAO.comp", true)->GetShader();
        m_SSAOComputePipeline = Flourish::ComputePipeline::Create(compCreateInfo);
    }

    void SceneRenderer::ClearRenderData()
    {
        for (u32 i = 0; i < Flourish::Context::FrameBufferCount(); i++)
        {
            m_BatchRenderData[i].IndirectBatches.clear();
            m_BatchRenderData[i].RenderedInstanceCount = 0;
            m_BatchRenderData[i].RenderedObjectCount = 0;
        }
    }

    Task SceneRenderer::Render(RenderScene* scene, EnvironmentMap* envMap, const Camera& camera, glm::vec3 cameraPosition, const SceneRenderSettings& renderSettings)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("SceneRenderer::Render");

        HE_ENGINE_ASSERT(scene, "Scene cannot be nullptr");
        
        if (m_ShouldResize)
            Resize();

        // If scenes have changed, clear previously computed batches so that no renders occur
        // since assets may possibly unload or resources are no longer valid
        if (scene != m_Scene)
            ClearRenderData();

        // Reset in-flight frame data
        m_SceneRenderSettings = renderSettings;
        m_Scene = scene;
        m_Camera = &camera;
        m_EnvironmentMap = envMap;

        // TODO: revisit this. Disabling previous-frame batch rendering for now because there is a lot of nuance in terms of
        // ghosting, culling, etc. that really isn't worth it right now
        m_UpdateFrameIndex = App::Get().GetFrameCount() % Flourish::Context::FrameBufferCount();
        m_RenderFrameIndex = m_UpdateFrameIndex;//(App::Get().GetFrameCount() - 1) % Flourish::Context::FrameBufferCount();
        m_BatchRenderData[m_UpdateFrameIndex].IndirectBatches.clear();
        m_BatchRenderData[m_UpdateFrameIndex].RenderedInstanceCount = 0;
        m_BatchRenderData[m_UpdateFrameIndex].RenderedObjectCount = 0;
        m_BatchRenderData[m_RenderFrameIndex].DeferredIndirectBatches.Clear();
        m_RenderBuffers.clear();
        for (auto& list : m_BatchRenderData[m_UpdateFrameIndex].EntityListPool)
            list.Clear();

        // Recalculate the indirect render batches for the next render
        Task updateTask = TaskManager::Schedule([this]()
        {
            CalculateBatches();
        }, Task::Priority::High, "SceneRenderer CalculateBatches");
        updateTask.Wait(); // Remove this when m_UpdateFrameIndex != m_RenderFrameIndex

        // Set the global data for this frame
        FrameData frameData = {
            camera.GetProjectionMatrix(),
            camera.GetViewMatrix(),
            glm::inverse(camera.GetProjectionMatrix()/* * camera.GetViewMatrix()*/),
            glm::vec4(cameraPosition, 1.f),
            { m_RenderWidth, m_RenderHeight },
            Flourish::Context::ReversedZBuffer(),
            m_SceneRenderSettings.CullEnable,
            m_SceneRenderSettings.BloomEnable,
            m_SceneRenderSettings.SSAOEnable,
            m_SceneRenderSettings.RenderPhysicsVolumes
        };
        m_FrameDataBuffer->SetElements(&frameData, 1, 0);

        // Update the light buffer with lights  (TODO: that are on screen)
        UpdateLightingBuffer();

        m_RenderEncoder = m_MainCommandBuffer->EncodeRenderCommands(m_MainFramebuffer.get());

        // Render the skybox if set
        if (m_EnvironmentMap)
            RenderEnvironmentMap();

        // Draw the grid if set
        m_RenderEncoder->StartNextSubpass();
        if (m_SceneRenderSettings.DrawGrid)   
            RenderGrid();

        // Batches pass
        m_RenderEncoder->StartNextSubpass();
        RenderBatches();
        
        // REMOVE AFTER UNCOMMENTING
        m_RenderEncoder->StartNextSubpass();
        
        // Composite pass
        m_RenderEncoder->StartNextSubpass();
        Composite();

        m_RenderEncoder->EndEncoding();

        // Copy ids texture
        if (m_SceneRenderSettings.CopyEntityIdsTextureToCPU)
            CopyEntityIdsTexture();
        
        // Debug physics rendering
        if (m_SceneRenderSettings.RenderPhysicsVolumes)
        {
            if (!m_PhysicsDebugRenderer)
                m_PhysicsDebugRenderer = CreateRef<PhysicsDebugRenderer>(m_RenderWidth, m_RenderHeight);
            //m_PhysicsDebugRenderer->Draw(m_Scene, *m_Camera);
        }

        // Submit
        if (m_SceneRenderSettings.RenderPhysicsVolumes)
            m_RenderBuffers.push_back({ m_MainCommandBuffer.get(), m_PhysicsDebugRenderer->GetCommandBuffer() });
        else
            m_RenderBuffers.push_back({ m_MainCommandBuffer.get() });
        
        // SSAO
        if (m_SceneRenderSettings.SSAOEnable)
            SSAO();

        // Bloom
        if (m_SceneRenderSettings.BloomEnable)
            Bloom();
            
        FinalComposite();
        
        return updateTask;
    }

    void SceneRenderer::UpdateLightingBuffer()
    {
        HE_PROFILE_FUNCTION();

        u32 lightIndex = 1;
        for (const auto& lightComp : m_Scene->GetLightComponents())
        {
            const auto& entityData = m_Scene->GetEntityData()[lightComp.EntityIndex];

            u32 offset = lightIndex * m_LightingDataBuffer->GetStride();

            if (lightComp.Data.LightType == LightComponent::Type::Disabled) continue;

            // Update the translation part of the light struct
            m_LightingDataBuffer->SetBytes(&entityData.Translation, sizeof(glm::vec3), offset);
            offset += sizeof(glm::vec4);

            // Update the light direction if the light is not a point light
            if (lightComp.Data.LightType != LightComponent::Type::Point)
            {
                // Negate the forward vector so it points in the direction of the light's +Z
                glm::vec3 forwardVector = -entityData.ForwardVec;
                m_LightingDataBuffer->SetBytes(&forwardVector, sizeof(forwardVector), offset);
            }
            offset += sizeof(glm::vec4);

            // Update the rest of the light data after the transform
            m_LightingDataBuffer->SetBytes(&lightComp.Data, sizeof(lightComp.Data), offset);

            lightIndex++;
        }

        // Update the first element of the light buffer to contain the number of lights
        float lightCount = static_cast<float>(lightIndex - 1);
        m_LightingDataBuffer->SetBytes(&lightCount, sizeof(float), 0);
    }

    void SceneRenderer::CalculateBatches()
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("SceneRenderer::CalculateBatches");

        bool async = m_SceneRenderSettings.AsyncAssetLoading;
        auto& batchData = m_BatchRenderData[m_UpdateFrameIndex];
        
        // TODO: use last frame's batches to render while calculating this frame's in parallel
        
        // Loop over each mesh component / submesh, hash the mesh & material, and place the entity in a batch
        // associated with the mesh & material. At this stage, Batch.First is unused and Batch.Count indicates
        // how many instances there are
        u32 batchIndex = 0;
        for (const auto& meshComp : m_Scene->GetMeshComponents())
        {
            const auto& entityData = m_Scene->GetEntityData()[meshComp.EntityIndex];

            // Compute max scale for calculating the bounding sphere
            // TODO: scale scale by some factor or some sort of predictive culling based on camera speed
            glm::vec3 scale = entityData.Scale;
            float maxScale = std::max(std::max(scale.x, scale.y), scale.z);

            // Skip invalid meshes
            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(meshComp.Data.Mesh, true, async);
            if (!meshAsset || !meshAsset->IsValid()) continue;

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);
               
                glm::vec4 boundingSphere = meshData.GetBoundingSphere();
                boundingSphere.w *= maxScale; // Extend the bounding sphere to fit the largest scale 
                if (m_SceneRenderSettings.CullEnable && 
                    !FrustumCull(boundingSphere, entityData.Transform))
                    continue;
                
                // Create a hash based on the submesh and its material if applicable
                u64 hash = meshComp.Data.Mesh ^ (i * 45787893);
                if (meshData.GetMaterialIndex() < meshComp.Data.Materials.Count())
                    hash ^= meshComp.Data.Materials[meshData.GetMaterialIndex()];

                // Get/create a batch associated with this hash
                auto& batch = batchData.IndirectBatches[hash];

                // Update the batch information if this is the first entity being added to it
                if (batch.Count == 0)
                {
                    // Retrieve a vector from the pool
                    batch.EntityListIndex = batchIndex;
                    if (batchIndex >= batchData.EntityListPool.Count())
                        batchData.EntityListPool.AddInPlace();

                    // Set the material & mesh associated with this batch
                    batch.Mesh = &meshData;
                    batch.Material = &meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()]; // default material
                    if (meshComp.Data.Materials.Count() > meshData.GetMaterialIndex())
                    {
                        auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(meshComp.Data.Materials[meshData.GetMaterialIndex()]);
                        if (materialAsset && materialAsset->IsValid())
                            batch.Material = &materialAsset->GetMaterial();
                    }

                    batchIndex++;
                }
                
                batch.Count++;
                batchData.RenderedInstanceCount++;

                // Push the associated entity to the associated vector from the pool
                batchData.EntityListPool[batch.EntityListIndex].AddInPlace(static_cast<u32>(meshComp.EntityIndex));
            }
        }

        // Loop over the calculated batches and populate the indirect buffer with draw commands. Because we are instancing, we need to make sure each object/material data
        // element gets placed contiguously for each indirect draw call. At this stage, Batch.First is the index of the indirect draw command in the buffer and
        // Batch.Count will equal 1 because it represents how many draw commands are in each batch
        u32 commandIndex = 0;
        u32 objectId = 0;
        for (auto& pair : batchData.IndirectBatches)
        {
            // Update the draw command index
            pair.second.First = commandIndex;

            // Popupate the indirect buffer
            IndexedIndirectCommand command = {
                pair.second.Mesh->GetIndexBuffer()->GetAllocatedCount(),
                pair.second.Count,
                0, 0, objectId
            };
            batchData.IndirectBuffer->SetElements(&command, 1, commandIndex);

            // Contiguiously set the instance data for each entity associated with this batch
            auto& entityList = batchData.EntityListPool[pair.second.EntityListIndex];
            for (u32 entity : entityList)
            {
                const auto& entityData = m_Scene->GetEntityData()[entity];

                // Object data
                ObjectData objectData = {
                    entityData.Transform,
                    { entityData.Id, 0.f, 0.f, 0.f }
                };
                batchData.ObjectDataBuffer->SetElements(&objectData, 1, objectId);

                // Material data
                if (pair.second.Material)
                {
                    auto& materialData = pair.second.Material->GetMaterialData();

                    auto albedoAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetAlbedoTexture(), true, async);
                    materialData.SetHasAlbedo(albedoAsset && albedoAsset->IsValid());

                    auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetMetallicRoughnessTexture(), true, async);
                    materialData.SetHasMetallicRoughness(metallicRoughnessAsset && metallicRoughnessAsset->IsValid());

                    auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetNormalTexture(), true, async);
                    materialData.SetHasNormal(normalAsset && normalAsset->IsValid());

                    auto emissiveAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetEmissiveTexture(), true, async);
                    materialData.SetHasEmissive(emissiveAsset && emissiveAsset->IsValid());

                    auto occlusionAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetOcclusionTexture(), true, async);
                    materialData.SetHasOcclusion(occlusionAsset && occlusionAsset->IsValid());

                    batchData.MaterialDataBuffer->SetElements(&materialData, 1, objectId);
                }
                else
                    batchData.MaterialDataBuffer->SetElements(&AssetManager::RetrieveAsset<MaterialAsset>("engine/DefaultMaterial.hemat", true)->GetMaterial().GetMaterialData(), 1, objectId);

                objectId++;
            }

            // Change the count to represent the number of draw commands
            pair.second.Count = 1;

            commandIndex++;
        }
        
        batchData.RenderedObjectCount = objectId;
    }

    void SceneRenderer::RenderEnvironmentMap()
    {
        /*
        m_RenderEncoder->BindPipeline("skybox");
        m_RenderEncoder->BindPipelineBufferResource(0, m_FrameDataBuffer.get(), 0, 0, 1);
        m_RenderEncoder->BindPipelineTextureResource(1, m_EnvironmentMap->GetEnvironmentCubemap());
        m_RenderEncoder->FlushPipelineBindings();

        auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("engine/DefaultCube.gltf", true);
        auto& meshData = meshAsset->GetSubmesh(0);

        m_RenderEncoder->BindVertexBuffer(meshData.GetVertexBuffer());
        m_RenderEncoder->BindIndexBuffer(meshData.GetIndexBuffer());
        m_RenderEncoder->DrawIndexed(
            meshData.GetIndexBuffer()->GetAllocatedCount(),
            0, 0, 1, 0
        );
        */
    }

    void SceneRenderer::RenderGrid()
    {
        /*
        // Bind grid pipeline
        m_RenderEncoder->BindPipeline("grid");

        // Bind frame data
        m_RenderEncoder->BindPipelineBufferResource(0, m_FrameDataBuffer.get(), 0, 0, 1);

        m_RenderEncoder->FlushPipelineBindings();

        // Draw
        m_RenderEncoder->SetLineWidth(2.f);
        m_RenderEncoder->BindVertexBuffer(m_GridVertices.get());
        m_RenderEncoder->BindIndexBuffer(m_GridIndices.get());
        m_RenderEncoder->DrawIndexed(
            m_GridIndices->GetAllocatedCount(),
            0, 0, 1, 0
        );
        */
    }
    
    void SceneRenderer::BindMaterial(Material* material)
    {
        /*
        HE_PROFILE_FUNCTION();

        auto& materialData = material->GetMaterialData();

        if (materialData.HasAlbedo())
        {
            auto albedoAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetAlbedoTexture());
            m_RenderEncoder->BindPipelineTextureResource(4, albedoAsset->GetTexture());
        }
        
        if (materialData.HasMetallicRoughness())
        {
            auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetMetallicRoughnessTexture());
            m_RenderEncoder->BindPipelineTextureResource(5, metallicRoughnessAsset->GetTexture());
        }

        if (materialData.HasNormal())
        {
            auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetNormalTexture());
            m_RenderEncoder->BindPipelineTextureResource(6, normalAsset->GetTexture());
        }

        if (materialData.HasEmissive())
        {
            auto emissiveAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetEmissiveTexture());
            m_RenderEncoder->BindPipelineTextureResource(7, emissiveAsset->GetTexture());
        }

        if (materialData.HasOcclusion())
        {
            auto occlusionAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetOcclusionTexture());
            m_RenderEncoder->BindPipelineTextureResource(8, occlusionAsset->GetTexture());
        }
        */
    }

    void SceneRenderer::BindPBRDefaults()
    {
        /*
        auto& batchData = m_BatchRenderData[m_RenderFrameIndex];

        // Bind frame data
        m_RenderEncoder->BindPipelineBufferResource(0, m_FrameDataBuffer.get(), 0, 0, 1);

        // Bind object data
        m_RenderEncoder->BindPipelineBufferResource(1, batchData.ObjectDataBuffer.get(), 0, 0, batchData.ObjectDataBuffer->GetAllocatedCount());

        // Bind material data
        m_RenderEncoder->BindPipelineBufferResource(2, batchData.MaterialDataBuffer.get(), 0, 0, batchData.MaterialDataBuffer->GetAllocatedCount());
        
        // Bind lighting data
        m_RenderEncoder->BindPipelineBufferResource(3, m_LightingDataBuffer.get(), 0, 0, m_LightingDataBuffer->GetAllocatedCount());

        // Default texture binds
        Flourish::Texture* defaultTexture = AssetManager::RetrieveAsset<TextureAsset>("engine/DefaultTexture.png", true)->GetTexture();
        m_RenderEncoder->BindPipelineTextureResource(4, defaultTexture);
        m_RenderEncoder->BindPipelineTextureResource(5, defaultTexture);
        m_RenderEncoder->BindPipelineTextureResource(6, defaultTexture);
        m_RenderEncoder->BindPipelineTextureResource(7, defaultTexture);
        m_RenderEncoder->BindPipelineTextureResource(8, defaultTexture);
        if (m_EnvironmentMap)
        {
            m_RenderEncoder->BindPipelineTextureResource(9, m_EnvironmentMap->GetIrradianceCubemap());
            m_RenderEncoder->BindPipelineTextureResource(10, m_EnvironmentMap->GetPrefilterCubemap());
            m_RenderEncoder->BindPipelineTextureResource(11, m_EnvironmentMap->GetBRDFTexture());
        }
        else
        {
            m_RenderEncoder->BindPipelineTextureResource(9, m_DefaultEnvironmentMap.get());
            m_RenderEncoder->BindPipelineTextureResource(10, m_DefaultEnvironmentMap.get());
            m_RenderEncoder->BindPipelineTextureLayerResource(11, m_DefaultEnvironmentMap.get(), 0, 0);
        }
        
        // Bind SSAO texture
        m_RenderEncoder->BindPipelineTextureResource(12, m_SSAOTexture.get());
        */
    }

    bool SceneRenderer::FrustumCull(glm::vec4 boundingSphere, const glm::mat4& transform)
    {
        float radius = boundingSphere.w;
        boundingSphere.w = 1.f;
        glm::vec3 center = transform * boundingSphere;
        for (int i = 0; i < 6; i++)
        {
            auto& plane = m_Camera->GetFrustumPlanes()[i];
            if (plane.x * center.x + plane.y * center.y + plane.z * center.z + plane.w <= -radius)
                return false;
        }
        
        return true;
    }

    void SceneRenderer::RenderBatches()
    {
        /*
        HE_PROFILE_FUNCTION();
        
        // Bind opaque PBR pipeline
        m_RenderEncoder->BindPipeline("pbr");

        // Bind defaults
        BindPBRDefaults();

        // Initial (opaque) batches pass
        auto& batchData = m_BatchRenderData[m_RenderFrameIndex];
        for (auto& pair : batchData.IndirectBatches)
        {
            auto& batch = pair.second;

            if (batch.Material)
            {
                if (batch.Material->IsTranslucent())
                {
                    batchData.DeferredIndirectBatches.AddInPlace(&batch);
                    continue;
                }
                BindMaterial(batch.Material);
            }

            m_RenderEncoder->FlushPipelineBindings();

            // Draw
            m_RenderEncoder->BindVertexBuffer(batch.Mesh->GetVertexBuffer());
            m_RenderEncoder->BindIndexBuffer(batch.Mesh->GetIndexBuffer());
            m_RenderEncoder->DrawIndexedIndirect(
                batchData.IndirectBuffer.get(), batch.First, batch.Count
            );
        }
        
        RenderText();
        
        // Secondary (translucent) batches pass
        m_RenderEncoder->StartNextSubpass();
        m_RenderEncoder->BindPipeline("pbrTpColor");
        BindPBRDefaults();

        for (auto batch : batchData.DeferredIndirectBatches)
        {
            if (batch->Material)
                BindMaterial(batch->Material);

            m_RenderEncoder->FlushPipelineBindings();

            // Draw
            m_RenderEncoder->BindVertexBuffer(batch->Mesh->GetVertexBuffer());
            m_RenderEncoder->BindIndexBuffer(batch->Mesh->GetIndexBuffer());
            m_RenderEncoder->DrawIndexedIndirect(
                batchData.IndirectBuffer.get(), batch->First, batch->Count
            );
        }
        */
    }
    
    void SceneRenderer::RenderText()
    {
        /*
        HE_PROFILE_FUNCTION();
        
        // Bind text PBR pipeline
        m_RenderEncoder->BindPipeline("pbrText");

        // Bind defaults
        BindPBRDefaults();

        // TODO: batch by font?
        MaterialData material;
        auto& batchData = m_BatchRenderData[m_RenderFrameIndex];
        for (auto& textComp : m_Scene->GetTextComponents())
        {
            const auto& entityData = m_Scene->GetEntityData()[textComp.EntityIndex];
            
            auto fontAsset = AssetManager::RetrieveAsset<FontAsset>(textComp.Data.Font, true, m_SceneRenderSettings.AsyncAssetLoading);
            if (!fontAsset || !fontAsset->IsValid()) continue;
            
            // Object data
            ObjectData objectData = {
                entityData.Transform,
                { entityData.Id, 0.f, 0.f, 0.f }
            };
            batchData.ObjectDataBuffer->SetElements(&objectData, 1, batchData.RenderedObjectCount);
            
            // Material data
            material.BaseColor = glm::vec4(textComp.Data.BaseColor, 1.f);
            material.EmissiveFactor = glm::vec4(textComp.Data.EmissiveFactor, 1.f);
            material.Scalars.x = textComp.Data.Metalness;
            material.Scalars.y = textComp.Data.Roughness;
            batchData.MaterialDataBuffer->SetElements(&material, 1, batchData.RenderedObjectCount);
            
            m_RenderEncoder->BindPipelineTextureResource(16, fontAsset->GetAtlasTexture());
            m_RenderEncoder->FlushPipelineBindings();
            
            // Draw
            m_RenderEncoder->BindVertexBuffer(textComp.Data.ComputedVertices.get());
            m_RenderEncoder->BindIndexBuffer(textComp.Data.ComputedIndices.get());
            m_RenderEncoder->DrawIndexed(
                textComp.Data.ComputedIndices->GetAllocatedCount(),
                0, 0, 1, batchData.RenderedObjectCount
            );
             
            batchData.RenderedObjectCount++;
        }
        */
    }

    void SceneRenderer::Composite()
    {
        /*
        // Bind alpha compositing pipeline
        m_RenderEncoder->BindPipeline("tpComposite");

        m_RenderEncoder->BindPipelineBufferResource(0, m_FrameDataBuffer.get(), 0, 0, 1);

        // Bind the input attachments from the transparent pass
        m_RenderEncoder->BindPipelineSubpassInputResource(1, { Flourish::SubpassAttachmentType::Color, 1 });
        m_RenderEncoder->BindPipelineSubpassInputResource(2, { Flourish::SubpassAttachmentType::Color, 2 });

        m_RenderEncoder->FlushPipelineBindings();

        // Draw the fullscreen triangle (TODO: make this compute)
        m_RenderEncoder->Draw(3, 0, 1, 0);
        */
    }

    void SceneRenderer::CopyEntityIdsTexture()
    {
        auto encoder = m_MainCommandBuffer->EncodeTransferCommands();
        encoder->CopyTextureToBuffer(m_EntityIdsTexture.get(), m_EntityIdsPixelBuffer.get());
        encoder->EndEncoding();

        // Async flushing because we don't need the results immediately
        m_EntityIdsPixelBuffer->Flush();
    }

    void SceneRenderer::SSAO()
    {
        /*
        m_SSAOData.KernelSize = m_SceneRenderSettings.SSAOKernelSize;
        m_SSAOData.Radius = m_SceneRenderSettings.SSAORadius;
        m_SSAOData.Bias = m_SceneRenderSettings.SSAOBias;
        m_SSAODataBuffer->SetElements(&m_SSAOData, 1, 0);

        auto encoder = m_SSAOCommandBuffer->EncodeComputeCommands();
        encoder->BindPipeline(m_SSAOComputePipeline.get());
        encoder->BindPipelineBufferResource(0, m_FrameDataBuffer.get(), 0, 0, 1);
        encoder->BindPipelineBufferResource(1, m_SSAODataBuffer.get(), 0, 0, 1);
        encoder->BindPipelineTextureResource(2, m_DepthTexture.get());
        encoder->BindPipelineTextureResource(3, m_SSAONoiseTexture.get());
        encoder->BindPipelineTextureResource(4, m_SSAOTexture.get());
        encoder->FlushPipelineBindings();
        encoder->Dispatch((m_RenderWidth / 16) + 1, (m_RenderHeight / 16) + 1, 1);
        encoder->EndEncoding();

        m_RenderBuffers.push_back({ m_SSAOCommandBuffer.get() });
        */
    }

    void SceneRenderer::Bloom()
    {
        /*
        // Downsample
        for (u32 i = 1; i < m_BloomMipCount; i++)
        {
            BloomData data = {
                {
                    i == 1 ? m_RenderOutputTexture->GetWidth() : m_BloomDownsampleBufferTexture->GetMipWidth(i - 1),
                    i == 1 ? m_RenderOutputTexture->GetHeight() : m_BloomDownsampleBufferTexture->GetMipHeight(i - 1),
                },
                { m_BloomDownsampleBufferTexture->GetMipWidth(i), m_BloomDownsampleBufferTexture->GetMipHeight(i) },
                m_SceneRenderSettings.BloomThreshold,
                m_SceneRenderSettings.BloomKnee,
                m_SceneRenderSettings.BloomSampleScale,
                i == 1
            };
            m_BloomDataBuffer->SetElements(&data, 1, i - 1);

            auto encoder = m_BloomCommandBuffer->EncodeComputeCommands();
            encoder->BindPipeline(m_BloomDownsampleComputePipeline.get());
            encoder->BindPipelineBufferResource(0, m_BloomDataBuffer.get(), 0, i - 1, 1);
            if (i == 1)
                encoder->BindPipelineTextureResource(1, m_RenderOutputTexture.get());
            else
                encoder->BindPipelineTextureLayerResource(1, m_BloomDownsampleBufferTexture.get(), 0, i - 1);
            encoder->BindPipelineTextureLayerResource(2, m_BloomDownsampleBufferTexture.get(), 0, i);
            encoder->FlushPipelineBindings();
            encoder->Dispatch((data.DstResolution.x / 16) + 1, (data.DstResolution.y / 16) + 1, 1);
            encoder->EndEncoding();
        }
        
        // Upsample
        for (u32 i = m_BloomMipCount - 2; i > 0; i--)
        {
            BloomData data = {
                { m_BloomDownsampleBufferTexture->GetMipWidth(i + 1), m_BloomDownsampleBufferTexture->GetMipHeight(i + 1) },
                { m_BloomUpsampleBufferTexture->GetMipWidth(i), m_BloomUpsampleBufferTexture->GetMipHeight(i) },
                m_SceneRenderSettings.BloomThreshold,
                m_SceneRenderSettings.BloomKnee,
                m_SceneRenderSettings.BloomSampleScale,
                false
            };
            m_BloomDataBuffer->SetElements(&data, 1, i + m_BloomMipCount);

            auto encoder = m_BloomCommandBuffer->EncodeComputeCommands();
            encoder->BindPipeline(m_BloomUpsampleComputePipeline.get());
            encoder->BindPipelineBufferResource(0, m_BloomDataBuffer.get(), 0, i + m_BloomMipCount, 1);
            if (i == m_BloomMipCount - 2)
                encoder->BindPipelineTextureLayerResource(1, m_BloomDownsampleBufferTexture.get(), 0, i + 1);
            else
                encoder->BindPipelineTextureLayerResource(1, m_BloomUpsampleBufferTexture.get(), 0, i + 1);
            encoder->BindPipelineTextureLayerResource(2, m_BloomDownsampleBufferTexture.get(), 0, i);
            encoder->BindPipelineTextureLayerResource(3, m_BloomUpsampleBufferTexture.get(), 0, i);
            encoder->FlushPipelineBindings();
            encoder->Dispatch((data.DstResolution.x / 16) + 1, (data.DstResolution.y / 16) + 1, 1);
            encoder->EndEncoding();
        }

        m_RenderBuffers.push_back({ m_BloomCommandBuffer.get() });
        */
    }
    
    void SceneRenderer::FinalComposite()
    {
        /*
        auto encoder = m_FinalCommandBuffer->EncodeComputeCommands();
        encoder->BindPipeline(m_FinalCompositeComputePipeline.get());
        encoder->BindPipelineBufferResource(0, m_FrameDataBuffer.get(), 0, 0, 1);
        encoder->BindPipelineTextureResource(1, m_FinalTexture.get());
        encoder->BindPipelineTextureResource(2, m_RenderOutputTexture.get());
        encoder->BindPipelineTextureResource(3, m_BloomUpsampleBufferTexture.get());
        if (m_SceneRenderSettings.RenderPhysicsVolumes)
            encoder->BindPipelineTextureResource(4, m_PhysicsDebugRenderer->GetFinalTexture());
        else
            encoder->BindPipelineTextureResource(4, m_RenderOutputTexture.get()); // Dummy texture
        encoder->FlushPipelineBindings();
        encoder->Dispatch((m_FinalTexture->GetWidth() / 16) + 1, (m_FinalTexture->GetHeight() / 16) + 1, 1);
        encoder->EndEncoding();

        m_RenderBuffers.push_back({ m_FinalCommandBuffer.get() });
        */
    }

    void SceneRenderer::InitializeGridBuffers()
    {
        // Default size (TODO: parameterize)
        u32 gridSize = 20;

        HVector<glm::vec3> vertices;
        vertices.Reserve(static_cast<size_t>(pow(gridSize + 1, 2)));
        HVector<u32> indices;
        
        // Calculate the grid with a line list
        glm::vec3 pos = { gridSize * -0.5f, 0.f, gridSize * -0.5f };
        u32 vertexIndex = 0;
        for (u32 i = 0; i <= gridSize; i++)
        {
            for (u32 j = 0; j <= gridSize; j++)
            {
                vertices.AddInPlace(pos);

                if (j != 0)
                {
                    indices.AddInPlace(vertexIndex);
                    indices.AddInPlace(vertexIndex - 1);
                }
                if (i != 0)
                {
                    indices.AddInPlace(vertexIndex);
                    indices.AddInPlace(vertexIndex - gridSize - 1);
                }

                pos.x += 1.f;
                vertexIndex++;
            }
            pos.x = gridSize * -0.5f;
            pos.z += 1.f;
        }

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Type = Flourish::BufferType::Vertex;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Static;
        bufCreateInfo.Layout = { Flourish::BufferDataType::Float3 };
        bufCreateInfo.ElementCount = vertices.Count();
        bufCreateInfo.InitialData = vertices.Data();
        bufCreateInfo.InitialDataSize = sizeof(float) * 3 * vertices.Count();
        m_GridVertices = Flourish::Buffer::Create(bufCreateInfo);

        bufCreateInfo.Type = Flourish::BufferType::Index;
        bufCreateInfo.Layout = { Flourish::BufferDataType::UInt };
        bufCreateInfo.ElementCount = indices.Count();
        bufCreateInfo.InitialData = indices.Data();
        bufCreateInfo.InitialDataSize = sizeof(u32) * indices.Count();
        m_GridIndices = Flourish::Buffer::Create(bufCreateInfo);
    }

    // https://learnopengl.com/Advanced-Lighting/SSAO
    void SceneRenderer::InitializeSSAOData()
    {
        // Generate random hemispherical perturbation vectors
        std::uniform_real_distribution<float> floats(0.f, 1.f);
        std::default_random_engine generator;
        for (u32 i = 0; i < 64; i++)
        {
            m_SSAOData.Samples[i].x = floats(generator) * 2.f - 1.f;
            m_SSAOData.Samples[i].y = floats(generator) * 2.f - 1.f;
            m_SSAOData.Samples[i].z = floats(generator);
            m_SSAOData.Samples[i].w = 0.f;

            m_SSAOData.Samples[i] = glm::normalize(m_SSAOData.Samples[i]);
            m_SSAOData.Samples[i] *= floats(generator);

            // Scale sample to distribute more towards the origin
            float scale = (float)i / 64.f; 
            scale = 0.1f + (scale * scale * 0.9f); // lerp(0.1, 1.0, scale * scale)
            m_SSAOData.Samples[i] *= scale;
        }

        // Generate random noise for the sample kernels
        std::vector<glm::vec4> noise;
        for (u32 i = 0; i < 16; i++)
        {
            noise.emplace_back(
                floats(generator) * 2.f - 1.f, 
                floats(generator) * 2.f - 1.f, 
                0.f,
                0.f
            );
        }

        // Create a texture from the random noise for later sampling
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = 4;
        texCreateInfo.Height = 4;
        texCreateInfo.Format = Flourish::ColorFormat::RGBA32_FLOAT;
        texCreateInfo.Usage = Flourish::TextureUsageType::Readonly;
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.InitialData = noise.data();
        texCreateInfo.InitialDataSize = noise.size() * sizeof(glm::vec4);
        texCreateInfo.SamplerState.UVWWrap = {
            Flourish::SamplerWrapMode::Repeat, Flourish::SamplerWrapMode::Repeat, Flourish::SamplerWrapMode::Repeat
        };
        m_SSAONoiseTexture = Flourish::Texture::Create(texCreateInfo);
    }
}
