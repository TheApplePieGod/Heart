#include "hepch.h"
#include "SceneRenderer.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"
#include "Heart/Core/Timing.h"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/GraphicsPipeline.h"
#include "Flourish/Api/ComputePipeline.h"
#include "Flourish/Api/ComputeTarget.h"
#include "Flourish/Api/RenderContext.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/Buffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/GraphicsCommandEncoder.h"
#include "Heart/Renderer/Material.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Core/Camera.h"
#include "Heart/Events/AppEvents.h"
#include "Heart/Events/WindowEvents.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Asset/MeshAsset.h"
#include "glm/gtc/matrix_transform.hpp"

namespace Heart
{
    SceneRenderer::SceneRenderer()
    {
        SubscribeToEmitter(&App::Get());
        SubscribeToEmitter(&Window::GetMainWindow()); // We manually handle window resizes here

        m_RenderWidth = Window::GetMainWindow().GetWidth();
        m_RenderHeight = Window::GetMainWindow().GetHeight();

        Initialize();
    }

    SceneRenderer::~SceneRenderer()
    {
        UnsubscribeFromEmitter(&App::Get());
        UnsubscribeFromEmitter(&Window::GetMainWindow());
        Shutdown();
    }

    void SceneRenderer::OnEvent(Event& event)
    {
        event.Map<AppGraphicsInitEvent>(HE_BIND_EVENT_FN(SceneRenderer::OnAppGraphicsInit));
        event.Map<AppGraphicsShutdownEvent>(HE_BIND_EVENT_FN(SceneRenderer::OnAppGraphicsShutdown));
        event.Map<WindowResizeEvent>(HE_BIND_EVENT_FN(SceneRenderer::OnWindowResize));
    }

    bool SceneRenderer::OnAppGraphicsInit(AppGraphicsInitEvent& event)
    {
        if (!m_Initialized)
            Initialize();
        return false;
    }

    bool SceneRenderer::OnAppGraphicsShutdown(AppGraphicsShutdownEvent& event)
    {
        Shutdown();
        return false;
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
        m_Initialized = true;

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

        // Initialize data buffers
        Flourish::BufferLayout frameDataLayout = {
            { Flourish::BufferDataType::Mat4 }, // proj matrix
            { Flourish::BufferDataType::Mat4 }, // view matrix
            { Flourish::BufferDataType::Float4 }, // camera pos
            { Flourish::BufferDataType::Float2 }, // screen size
            { Flourish::BufferDataType::Bool }, // reverse depth
            { Flourish::BufferDataType::Bool }, // bloom enable
            { Flourish::BufferDataType::Bool }, // cull enable
            { Flourish::BufferDataType::Float }, // padding
            { Flourish::BufferDataType::Float2 } // padding
        };
        Flourish::BufferLayout bloomDataLayout = {
            { Flourish::BufferDataType::UInt }, // mip level
            { Flourish::BufferDataType::Bool }, // reverse depth
            { Flourish::BufferDataType::Float }, // blur scale
            { Flourish::BufferDataType::Float } // blur strength
        };
        Flourish::BufferLayout objectDataLayout = {
            { Flourish::BufferDataType::Mat4 }, // transform
            { Flourish::BufferDataType::Float4 }, // [0]: entityId
            { Flourish::BufferDataType::Float4 }, // boundingSphere
        };
        Flourish::BufferLayout materialDataLayout = {
            { Flourish::BufferDataType::Float4 }, // position
            { Flourish::BufferDataType::Float4 }, // emissive factor
            { Flourish::BufferDataType::Float4 }, // texcoord transform
            { Flourish::BufferDataType::Float4 }, // has PBR textures
            { Flourish::BufferDataType::Float4 }, // has textures
            { Flourish::BufferDataType::Float4 } // scalars
        };
        Flourish::BufferLayout lightingDataLayout = {
            { Flourish::BufferDataType::Float4 }, // position
            { Flourish::BufferDataType::Float4 }, // rotation
            { Flourish::BufferDataType::Float4 }, // color
            { Flourish::BufferDataType::UInt }, // light type
            { Flourish::BufferDataType::Float }, // constant attenuation
            { Flourish::BufferDataType::Float }, // linear attenuation
            { Flourish::BufferDataType::Float } // quadratic attenuation
        };
        Flourish::BufferLayout indirectDataLayout = {
            { Flourish::BufferDataType::UInt }, // index count
            { Flourish::BufferDataType::UInt }, // instance count
            { Flourish::BufferDataType::UInt }, // first index
            { Flourish::BufferDataType::Int }, // vertex offset
            { Flourish::BufferDataType::UInt }, // first instance
            { Flourish::BufferDataType::UInt }, // padding
            { Flourish::BufferDataType::Float2 } // padding
        };
        Flourish::BufferLayout instanceDataLayout = {
            { Flourish::BufferDataType::UInt }, // object id
            { Flourish::BufferDataType::UInt }, // batch id
            { Flourish::BufferDataType::Float2 }, // padding
        };
        Flourish::BufferLayout cullDataLayout = {
            { Flourish::BufferDataType::Float4 }, // frustum planes [0]
            { Flourish::BufferDataType::Float4 }, // frustum planes [1]
            { Flourish::BufferDataType::Float4 }, // frustum planes [2]
            { Flourish::BufferDataType::Float4 }, // frustum planes [3]
            { Flourish::BufferDataType::Float4 }, // frustum planes [4]
            { Flourish::BufferDataType::Float4 }, // frustum planes [5]
            { Flourish::BufferDataType::Float4 } // [0]: drawCount
        };
        Flourish::BufferLayout testDataLayout = {
            { Flourish::BufferDataType::Float2 }, // src res
            { Flourish::BufferDataType::Float2 }, // dst res
            { Flourish::BufferDataType::Float }, // threshold
            { Flourish::BufferDataType::Float }, // filter radius
            { Flourish::BufferDataType::Float2 }, // padding
            { Flourish::BufferDataType::Float4 }, 
            { Flourish::BufferDataType::Float4 } 
        };

        u32 maxObjects = 10000;
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Type = Flourish::BufferType::Uniform;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Layout = frameDataLayout;
        bufCreateInfo.ElementCount = 1;
        m_FrameDataBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Layout = cullDataLayout;
        m_CullDataBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Layout = testDataLayout;
        bufCreateInfo.ElementCount = 20;
        m_TestBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Layout = bloomDataLayout;
        bufCreateInfo.ElementCount = 50;
        m_BloomDataBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Layout = lightingDataLayout;
        bufCreateInfo.ElementCount = 500;
        m_LightingDataBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Layout = objectDataLayout;
        bufCreateInfo.ElementCount = maxObjects;
        m_ObjectDataBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Layout = materialDataLayout;
        m_MaterialDataBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Layout = instanceDataLayout;
        m_InstanceDataBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Layout = { Flourish::BufferDataType::Float4 };
        m_FinalInstanceBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Type = Flourish::BufferType::Indirect;
        bufCreateInfo.Layout = indirectDataLayout;
        m_IndirectBuffer = Flourish::Buffer::Create(bufCreateInfo);

        InitializeGridBuffers();
        CreateTextures();
        CreateFramebuffers();

        // Create command buffer
        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.MaxEncoders = 20; // TODO: look at this
        m_MainCommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);
        cbCreateInfo.MaxEncoders = 30; 
        m_TestCommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);
        cbCreateInfo.MaxEncoders = 2; 
        m_FinalCommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        // Create compute pipelines
        Flourish::ComputePipelineCreateInfo compCreateInfo;
        compCreateInfo.ComputeShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/IndirectCull.comp", true)->GetShader();
        m_ComputeCullPipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        compCreateInfo.ComputeShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/ComputeBloomDownsample.comp", true)->GetShader();
        m_TestDownsampleComputePipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        compCreateInfo.ComputeShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/ComputeBloomUpsample.comp", true)->GetShader();
        m_TestUpsampleComputePipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        compCreateInfo.ComputeShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/FinalComposite.comp", true)->GetShader();
        m_FinalComputePipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        
        m_TestComputeTarget = Flourish::ComputeTarget::Create();
        m_FinalComputeTarget = Flourish::ComputeTarget::Create();
    }

    void SceneRenderer::Shutdown()
    {
        m_Initialized = false;

        CleanupFramebuffers();
        
        m_DefaultEnvironmentMap.reset();
        CleanupTextures();

        m_FrameDataBuffer.reset();
        m_BloomDataBuffer.reset();
        m_ObjectDataBuffer.reset();
        m_MaterialDataBuffer.reset();
        m_LightingDataBuffer.reset();

        m_ComputeCullPipeline.reset();
        m_CullDataBuffer.reset();
        m_InstanceDataBuffer.reset();
        m_FinalInstanceBuffer.reset();
        m_IndirectBuffer.reset();

        m_GridVertices.reset();
        m_GridIndices.reset();
        
        m_MainCommandBuffer.reset();
    }

    void SceneRenderer::Resize()
    {
        if (!m_Initialized) return;
        m_ShouldResize = false;

        CleanupFramebuffers();
        CleanupTextures();

        CreateTextures();
        CreateFramebuffers();
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
        m_PreBloomTexture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.MipCount = m_BloomMipCount;
        m_BrightColorsTexture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.MipCount = m_BloomMipCount - 1;
        m_BloomUpsampleBufferTexture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Usage = Flourish::TextureUsageType::ComputeTarget;
        m_FinalTexture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.Usage = Flourish::TextureUsageType::RenderTarget;
        texCreateInfo.Format = Flourish::ColorFormat::R32_FLOAT;
        m_EntityIdsTexture = Flourish::Texture::Create(texCreateInfo);

        texCreateInfo.Usage = Flourish::TextureUsageType::ComputeTarget;
        texCreateInfo.MipCount = m_BloomMipCount;
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge };
        m_BloomBufferTexture = Flourish::Texture::Create(texCreateInfo);
        m_TestTexture = Flourish::Texture::Create(texCreateInfo);
    }

    void SceneRenderer::CleanupTextures()
    {
        m_PreBloomTexture.reset();
        m_BrightColorsTexture.reset();
        m_BloomBufferTexture.reset();
        m_BloomUpsampleBufferTexture.reset();
        
        m_FinalTexture.reset();
        m_EntityIdsTexture.reset();
    }

    void SceneRenderer::CreateFramebuffers()
    {
        // ------------------------------------------------------------------
        // Main framebuffer: handles majority of the rendering
        // ------------------------------------------------------------------
        {
            Flourish::RenderPassCreateInfo rpCreateInfo;
            rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
            rpCreateInfo.ColorAttachments.push_back({ m_EntityIdsTexture->GetColorFormat() });    // Entity ids           [0]
            rpCreateInfo.ColorAttachments.push_back({ Flourish::ColorFormat::RGBA16_FLOAT });     // Transparency data    [1]
            rpCreateInfo.ColorAttachments.push_back({ Flourish::ColorFormat::R16_FLOAT });        // Transparency data    [2]
            rpCreateInfo.ColorAttachments.push_back({ m_PreBloomTexture->GetColorFormat() });     // Pre-bloom target     [3]
            rpCreateInfo.ColorAttachments.push_back({ m_BrightColorsTexture->GetColorFormat() }); // Bright colors target [4]
            rpCreateInfo.DepthAttachments.push_back({});
            rpCreateInfo.Subpasses.push_back({ // Environment map
                {},
                { { Flourish::SubpassAttachmentType::Color, 3 }, { Flourish::SubpassAttachmentType::Color, 4 } }
            });
            rpCreateInfo.Subpasses.push_back({ // Grid
                {},
                { { Flourish::SubpassAttachmentType::Color, 3 } }
            });
            rpCreateInfo.Subpasses.push_back({ // Opaque
                {},
                { { Flourish::SubpassAttachmentType::Depth, 0 }, { Flourish::SubpassAttachmentType::Color, 3 }, { Flourish::SubpassAttachmentType::Color, 4 }, { Flourish::SubpassAttachmentType::Color, 0 } } 
            });
            rpCreateInfo.Subpasses.push_back({ // Transparent color
                {},
                { { Flourish::SubpassAttachmentType::Depth, 0 }, { Flourish::SubpassAttachmentType::Color, 0 }, { Flourish::SubpassAttachmentType::Color, 1 }, { Flourish::SubpassAttachmentType::Color, 2 } } 
            });
            rpCreateInfo.Subpasses.push_back({ // Composite
                { { Flourish::SubpassAttachmentType::Color, 1 }, { Flourish::SubpassAttachmentType::Color, 2 } },
                { { Flourish::SubpassAttachmentType::Depth, 0 }, { Flourish::SubpassAttachmentType::Color, 3 }, { Flourish::SubpassAttachmentType::Color, 4 } } 
            });
            m_MainRenderPass = Flourish::RenderPass::Create(rpCreateInfo);
            
            Flourish::FramebufferCreateInfo fbCreateInfo;
            fbCreateInfo.RenderPass = m_MainRenderPass;
            fbCreateInfo.Width = m_RenderWidth;
            fbCreateInfo.Height = m_RenderHeight;
            fbCreateInfo.ColorAttachments.push_back({ { -1.f, 0.f, 0.f, 0.f }, m_EntityIdsTexture });   // Entity ids           [0]
            fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f } });                        // Transparency data    [1]
            fbCreateInfo.ColorAttachments.push_back({ { 1.f, 0.f, 0.f, 0.f } });                        // Transparency data    [2]
            fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_PreBloomTexture });     // Pre-bloom target     [3]
            fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_BrightColorsTexture }); // Bright colors target [4]
            fbCreateInfo.DepthAttachments.push_back({});
            m_MainFramebuffer = Flourish::Framebuffer::Create(fbCreateInfo);
            
            Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
            pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/Grid.vert", true)->GetShader();
            pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/Grid.frag", true)->GetShader();
            pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::LineList;
            pipelineCreateInfo.VertexLayout = { Flourish::BufferDataType::Float3 };
            pipelineCreateInfo.VertexInput = true;
            pipelineCreateInfo.BlendStates = { { false } };
            pipelineCreateInfo.DepthTest = false;
            pipelineCreateInfo.DepthWrite = false;
            pipelineCreateInfo.CullMode = Flourish::CullMode::None;
            pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
            pipelineCreateInfo.CompatibleSubpasses = { 1 };
            m_MainRenderPass->CreatePipeline("grid", pipelineCreateInfo);
            pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/Skybox.vert", true)->GetShader();
            pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/Skybox.frag", true)->GetShader();
            pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
            pipelineCreateInfo.VertexLayout = Heart::Mesh::GetVertexLayout();
            pipelineCreateInfo.BlendStates = { { false }, { false } };
            pipelineCreateInfo.DepthTest = false;
            pipelineCreateInfo.DepthWrite = false;
            pipelineCreateInfo.CullMode = Flourish::CullMode::Frontface;
            pipelineCreateInfo.CompatibleSubpasses = { 0 };
            m_MainRenderPass->CreatePipeline("skybox", pipelineCreateInfo);
            pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/PBR.vert", true)->GetShader();
            pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/PBR.frag", true)->GetShader();
            pipelineCreateInfo.BlendStates = { { false }, { false }, { false } };
            pipelineCreateInfo.DepthTest = true;
            pipelineCreateInfo.DepthWrite = true;
            pipelineCreateInfo.CullMode = Flourish::CullMode::Backface;
            pipelineCreateInfo.CompatibleSubpasses = { 2 };
            m_MainRenderPass->CreatePipeline("pbr", pipelineCreateInfo);
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
                { true, Flourish::BlendFactor::OneMinusSrcAlpha, Flourish::BlendFactor::SrcAlpha, Flourish::BlendFactor::SrcAlpha, Flourish::BlendFactor::OneMinusSrcAlpha },
                { false }
            };
            pipelineCreateInfo.DepthWrite = false;
            pipelineCreateInfo.CompatibleSubpasses = { 4 };
            m_MainRenderPass->CreatePipeline("tpComposite", pipelineCreateInfo);
        }
    }

    void SceneRenderer::CleanupFramebuffers()
    {
        m_MainFramebuffer.reset();
    }

    void SceneRenderer::RenderScene(Scene* scene, const Camera& camera, glm::vec3 cameraPosition, const SceneRenderSettings& renderSettings)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("SceneRenderer::RenderScene");

        HE_ENGINE_ASSERT(scene, "Scene cannot be nullptr");

        if (m_ShouldResize)
            Resize();

        // Reset in-flight frame data
        m_RenderEncoder = m_MainCommandBuffer->EncodeRenderCommands(m_MainFramebuffer.get());
        m_SceneRenderSettings = renderSettings;
        m_Scene = scene;
        m_Camera = &camera;
        m_EnvironmentMap = scene->GetEnvironmentMap();
        m_IndirectBatches.clear();
        m_DeferredIndirectBatches.Clear();
        m_RenderBuffers.clear();
        for (auto& list : m_EntityListPool)
            list.Clear();

        // Update entity id cpu copy status
        // m_MainFramebuffer->UpdateColorAttachmentCPUVisibliity(0, m_SceneRenderSettings.CopyEntityIdsTextureToCPU);

        // Set the global data for this frame
        m_SceneRenderSettings.CullEnable = false;
        FrameData frameData = {
            camera.GetProjectionMatrix(), camera.GetViewMatrix(), glm::vec4(cameraPosition, 1.f),
            { m_RenderWidth, m_RenderHeight },
            Flourish::Context::ReversedZBuffer(),
            m_SceneRenderSettings.CullEnable,
            m_SceneRenderSettings.BloomEnable
        };
        m_FrameDataBuffer->SetElements(&frameData, 1, 0);

        // Update the light buffer with lights  (TODO: that are on screen)
        UpdateLightingBuffer();

        // Recalculate the indirect render batches
        CalculateBatches();

        // Run the cull shader if enabled
        /*
        if (m_SceneRenderSettings.CullEnable)
        {
            SetupCullCompute();
            m_MainFramebuffer->Bind(m_ComputeCullPipeline.get());
        }
        else
            m_MainFramebuffer->Bind();
        */

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

        // Composite pass
        m_RenderEncoder->StartNextSubpass();
        Composite();

        // Submit the framebuffer
        m_RenderEncoder->EndEncoding();
        m_RenderBuffers.push_back({ m_MainCommandBuffer.get() });

        // Bloom
        if (m_SceneRenderSettings.BloomEnable)
            Bloom();
        
        FinalComposite();
    }

    void SceneRenderer::UpdateLightingBuffer()
    {
        HE_PROFILE_FUNCTION();

        u32 lightIndex = 1;
        auto view = m_Scene->GetRegistry().view<TransformComponent, LightComponent>();
        for (auto entityHandle : view)
        {
            Entity entity = { m_Scene, entityHandle };
            auto [transform, light] = view.get<TransformComponent, LightComponent>(entityHandle);
            u32 offset = lightIndex * m_LightingDataBuffer->GetLayout().GetStride();

            if (light.LightType == LightComponent::Type::Disabled) continue;

            // Update the translation part of the light struct
            glm::vec3 worldPos = entity.GetWorldPosition();
            m_LightingDataBuffer->SetBytes(&worldPos, sizeof(glm::vec3), offset);
            offset += sizeof(glm::vec4);

            // Update the light direction if the light is not a point light
            if (light.LightType != LightComponent::Type::Point)
            {
                // Negate the forward vector so it points in the direction of the light's +Z
                glm::vec3 forwardVector = -entity.GetWorldForwardVector();
                m_LightingDataBuffer->SetBytes(&forwardVector, sizeof(forwardVector), offset);
            }
            offset += sizeof(glm::vec4);

            // Update the rest of the light data after the transform
            m_LightingDataBuffer->SetBytes(&light, sizeof(light), offset);

            lightIndex++;
        }

        // Update the first element of the light buffer to contain the number of lights
        float lightCount = static_cast<float>(lightIndex - 1);
        m_LightingDataBuffer->SetBytes(&lightCount, sizeof(float), 0);
    }

    void SceneRenderer::CalculateBatches()
    {
        HE_PROFILE_FUNCTION();

        m_RenderedInstanceCount = 0;
        bool async = m_SceneRenderSettings.AsyncAssetLoading;

        // Loop over each mesh component / submesh, hash the mesh & material, and place the entity in a batch
        // associated with the mesh & material. At this stage, Batch.First is unused and Batch.Count indicates
        // how many instances there are
        u32 batchIndex = 0;
        auto group = m_Scene->GetRegistry().group<TransformComponent, MeshComponent>();
        for (auto entity : group)
        {
            auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

            // Skip invalid meshes
            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(mesh.Mesh, async);
            if (!meshAsset || !meshAsset->IsValid()) continue;

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);

                // Create a hash based on the submesh and its material if applicable
                u64 hash = mesh.Mesh ^ (i * 45787893);
                if (meshData.GetMaterialIndex() < mesh.Materials.Count())
                    hash ^= mesh.Materials[meshData.GetMaterialIndex()];

                // Get/create a batch associated with this hash
                auto& batch = m_IndirectBatches[hash];

                // Update the batch information if this is the first entity being added to it
                if (batch.Count == 0)
                {
                    // Retrieve a vector from the pool
                    batch.EntityListIndex = batchIndex;
                    if (batchIndex >= m_EntityListPool.Count())
                        m_EntityListPool.AddInPlace();

                    // Set the material & mesh associated with this batch
                    batch.Mesh = &meshData;
                    batch.Material = &meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()]; // default material
                    if (mesh.Materials.Count() > meshData.GetMaterialIndex())
                    {
                        auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(mesh.Materials[meshData.GetMaterialIndex()]);
                        if (materialAsset && materialAsset->IsValid())
                            batch.Material = &materialAsset->GetMaterial();
                    }

                    batchIndex++;
                }
                
                batch.Count++;
                m_RenderedInstanceCount++;

                // Push the associated entity to the associated vector from the pool
                m_EntityListPool[batch.EntityListIndex].AddInPlace(static_cast<u32>(entity));
            }
        }

        // Loop over the calculated batches and populate the indirect buffer with draw commands. Because we are instancing, we need to make sure each object/material data
        // element gets placed contiguously for each indirect draw call. At this stage, Batch.First is the index of the indirect draw command in the buffer and
        // Batch.Count will equal 1 because it represents how many draw commands are in each batch
        u32 commandIndex = 0;
        u32 objectId = 0;
        for (auto& pair : m_IndirectBatches)
        {
            // Update the draw command index
            pair.second.First = commandIndex;

            // Popupate the indirect buffer
            IndexedIndirectCommand command = {
                pair.second.Mesh->GetIndexBuffer()->GetAllocatedCount(),
                m_SceneRenderSettings.CullEnable ? 0 : pair.second.Count, // If we are culling, we set instance count to zero because the shader will populate it
                0, 0, objectId
            };
            m_IndirectBuffer->SetElements(&command, 1, commandIndex);

            // Contiguiously set the instance data for each entity associated with this batch
            auto& entityList = m_EntityListPool[pair.second.EntityListIndex];
            for (auto& _entity : entityList)
            {
                Entity entity = { m_Scene, _entity };

                // Object data
                glm::vec3 scale = entity.GetScale();
                glm::vec4 boundingSphere = pair.second.Mesh->GetBoundingSphere();
                boundingSphere.w *= std::max(std::max(scale.x, scale.y), scale.z); // Extend the bounding sphere to fit the largest scale 
                ObjectData objectData = {
                    m_Scene->GetEntityCachedTransform(entity),
                    { _entity, 0.f, 0.f, 0.f },
                    boundingSphere
                };
                m_ObjectDataBuffer->SetElements(&objectData, 1, objectId);

                // Populate the instance data buffer if we are culling
                if (m_SceneRenderSettings.CullEnable)
                {
                    InstanceData instanceData = {
                        objectId, commandIndex
                    };
                    m_InstanceDataBuffer->SetElements(&instanceData, 1, objectId);
                }

                // Material data
                if (pair.second.Material)
                {
                    auto& materialData = pair.second.Material->GetMaterialData();

                    auto albedoAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetAlbedoTexture(), async);
                    materialData.SetHasAlbedo(albedoAsset && albedoAsset->IsValid());

                    auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetMetallicRoughnessTexture(), async);
                    materialData.SetHasMetallicRoughness(metallicRoughnessAsset && metallicRoughnessAsset->IsValid());

                    auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetNormalTexture(), async);
                    materialData.SetHasNormal(normalAsset && normalAsset->IsValid());

                    auto emissiveAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetEmissiveTexture(), async);
                    materialData.SetHasEmissive(emissiveAsset && emissiveAsset->IsValid());

                    auto occlusionAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetOcclusionTexture(), async);
                    materialData.SetHasOcclusion(occlusionAsset && occlusionAsset->IsValid());

                    m_MaterialDataBuffer->SetElements(&materialData, 1, objectId);
                }
                else
                    m_MaterialDataBuffer->SetElements(&AssetManager::RetrieveAsset<MaterialAsset>("engine/DefaultMaterial.hemat", true)->GetMaterial().GetMaterialData(), 1, objectId);

                objectId++;
            }

            // Change the count to represent the number of draw commands
            pair.second.Count = 1;

            commandIndex++;
        }
    }

    void SceneRenderer::SetupCullCompute()
    {
        /*
        CullData cullData = {
            m_Camera->GetFrustumPlanes(),
            { m_RenderedInstanceCount, 0.f, 0.f, 0.f }
        };
        m_CullDataBuffer->SetElements(&cullData, 1, 0);

        m_ComputeCullPipeline->Bind();
        m_ComputeCullPipeline->BindShaderBufferResource(0, 0, 1, m_CullDataBuffer.get());
        m_ComputeCullPipeline->BindShaderBufferResource(1, 0, m_ObjectDataBuffer->GetAllocatedCount(), m_ObjectDataBuffer.get());
        m_ComputeCullPipeline->BindShaderBufferResource(2, 0, m_IndirectBuffer->GetAllocatedCount(), m_IndirectBuffer.get());
        m_ComputeCullPipeline->BindShaderBufferResource(3, 0, m_InstanceDataBuffer->GetAllocatedCount(), m_InstanceDataBuffer.get());
        m_ComputeCullPipeline->BindShaderBufferResource(4, 0, m_FinalInstanceBuffer->GetAllocatedCount(), m_FinalInstanceBuffer.get());
        m_ComputeCullPipeline->FlushBindings();

        m_ComputeCullPipeline->SetDispatchCountX(m_RenderedInstanceCount / 128 + 1);
        */
    }

    void SceneRenderer::RenderEnvironmentMap()
    {
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
            0, 0, 1
        );
    }

    void SceneRenderer::RenderGrid()
    {
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
            0, 0, 1
        );
    }
    
    void SceneRenderer::BindMaterial(Material* material)
    {
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
    }

    void SceneRenderer::BindPBRDefaults()
    {
        // Bind frame data
        m_RenderEncoder->BindPipelineBufferResource(0, m_FrameDataBuffer.get(), 0, 0, 1);

        // Bind object data
        m_RenderEncoder->BindPipelineBufferResource(1, m_ObjectDataBuffer.get(), 0, 0, m_ObjectDataBuffer->GetAllocatedCount());

        // Bind culled instance map data
        m_RenderEncoder->BindPipelineBufferResource(12, m_FinalInstanceBuffer.get(), 0, 0, m_FinalInstanceBuffer->GetAllocatedCount());

        // Bind material data
        m_RenderEncoder->BindPipelineBufferResource(2, m_MaterialDataBuffer.get(), 0, 0, m_MaterialDataBuffer->GetAllocatedCount());
        
        // Bind lighting data
        m_RenderEncoder->BindPipelineBufferResource(3, m_LightingDataBuffer.get(), 0, 0, m_LightingDataBuffer->GetAllocatedCount());

        // Default texture binds
        m_RenderEncoder->BindPipelineTextureResource(4, AssetManager::RetrieveAsset<TextureAsset>("engine/DefaultTexture.png", true)->GetTexture());
        m_RenderEncoder->BindPipelineTextureResource(5, AssetManager::RetrieveAsset<TextureAsset>("engine/DefaultTexture.png", true)->GetTexture());
        m_RenderEncoder->BindPipelineTextureResource(6, AssetManager::RetrieveAsset<TextureAsset>("engine/DefaultTexture.png", true)->GetTexture());
        m_RenderEncoder->BindPipelineTextureResource(7, AssetManager::RetrieveAsset<TextureAsset>("engine/DefaultTexture.png", true)->GetTexture());
        m_RenderEncoder->BindPipelineTextureResource(8, AssetManager::RetrieveAsset<TextureAsset>("engine/DefaultTexture.png", true)->GetTexture());
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
    }

    void SceneRenderer::RenderBatches()
    {
        HE_PROFILE_FUNCTION();
        
        // Bind opaque PBR pipeline
        m_RenderEncoder->BindPipeline("pbr");

        // Bind defaults
        BindPBRDefaults();

        // Initial (opaque) batches pass
        for (auto& pair : m_IndirectBatches)
        {
            auto& batch = pair.second;

            if (batch.Material)
            {
                if (batch.Material->IsTranslucent())
                {
                    m_DeferredIndirectBatches.AddInPlace(&batch);
                    continue;
                }
                BindMaterial(batch.Material);
            }

            m_RenderEncoder->FlushPipelineBindings();

            // Draw
            m_RenderEncoder->BindVertexBuffer(batch.Mesh->GetVertexBuffer());
            m_RenderEncoder->BindIndexBuffer(batch.Mesh->GetIndexBuffer());
            m_RenderEncoder->DrawIndexedIndirect(
                m_IndirectBuffer.get(), batch.First, batch.Count
            );
        }

        // Secondary (translucent) batches pass
        m_RenderEncoder->StartNextSubpass();
        m_RenderEncoder->BindPipeline("pbrTpColor");
        BindPBRDefaults();

        for (auto batch : m_DeferredIndirectBatches)
        {
            if (batch->Material)
                BindMaterial(batch->Material);

            m_RenderEncoder->FlushPipelineBindings();

            // Draw
            m_RenderEncoder->BindVertexBuffer(batch->Mesh->GetVertexBuffer());
            m_RenderEncoder->BindIndexBuffer(batch->Mesh->GetIndexBuffer());
            m_RenderEncoder->DrawIndexedIndirect(
                m_IndirectBuffer.get(), batch->First, batch->Count
            );
        }
    }

    void SceneRenderer::Composite()
    {
        // Bind alpha compositing pipeline
        m_RenderEncoder->BindPipeline("tpComposite");

        m_RenderEncoder->BindPipelineBufferResource(0, m_FrameDataBuffer.get(), 0, 0, 1);

        // Bind the input attachments from the transparent pass
        m_RenderEncoder->BindPipelineSubpassInputResource(1, { Flourish::SubpassAttachmentType::Color, 1 });
        m_RenderEncoder->BindPipelineSubpassInputResource(2, { Flourish::SubpassAttachmentType::Color, 2 });

        m_RenderEncoder->FlushPipelineBindings();

        // Draw the fullscreen triangle
        m_RenderEncoder->Draw(3, 0, 1);
    }

    void SceneRenderer::Bloom()
    {
        // Downsample
        u32 downMipCount = m_BloomBufferTexture->GetMipCount();
        for (u32 i = 1; i < downMipCount; i++)
        {
            TestData testData = {
                {
                    i == 1 ? m_PreBloomTexture->GetWidth() : m_BloomBufferTexture->GetMipWidth(i - 1),
                    i == 1 ? m_PreBloomTexture->GetHeight() : m_BloomBufferTexture->GetMipHeight(i - 1),
                },
                { m_BloomBufferTexture->GetMipWidth(i), m_BloomBufferTexture->GetMipHeight(i) },
                m_SceneRenderSettings.BloomThreshold,
                m_SceneRenderSettings.BloomKnee,
                m_SceneRenderSettings.BloomSampleScale,
                i == 1
            };
            m_TestBuffer->SetElements(&testData, 1, i - 1);

            auto encoder = m_TestCommandBuffer->EncodeComputeCommands(m_TestComputeTarget.get());
            encoder->BindPipeline(m_TestDownsampleComputePipeline.get());
            encoder->BindPipelineBufferResource(0, m_TestBuffer.get(), 0, i - 1, 1);
            if (i == 1)
                encoder->BindPipelineTextureResource(1, m_PreBloomTexture.get());
            else
                encoder->BindPipelineTextureLayerResource(1, m_BloomBufferTexture.get(), 0, i - 1);
            encoder->BindPipelineTextureLayerResource(2, m_BloomBufferTexture.get(), 0, i);
            encoder->FlushPipelineBindings();
            encoder->Dispatch((testData.DstResolution.x / 16) + 1, (testData.DstResolution.y / 16) + 1, 1);
            encoder->EndEncoding();
        }
        
        // Upsample
        u32 upMipCount = m_TestTexture->GetMipCount();
        for (u32 i = upMipCount - 2; i > 0; i--)
        {
            TestData testData = {
                { m_BloomBufferTexture->GetMipWidth(i + 1), m_BloomBufferTexture->GetMipHeight(i + 1) },
                { m_TestTexture->GetMipWidth(i), m_TestTexture->GetMipHeight(i) },
                m_SceneRenderSettings.BloomThreshold,
                m_SceneRenderSettings.BloomKnee,
                m_SceneRenderSettings.BloomSampleScale,
                false
            };
            m_TestBuffer->SetElements(&testData, 1, i + upMipCount);

            auto encoder = m_TestCommandBuffer->EncodeComputeCommands(m_TestComputeTarget.get());
            encoder->BindPipeline(m_TestUpsampleComputePipeline.get());
            encoder->BindPipelineBufferResource(0, m_TestBuffer.get(), 0, i + upMipCount, 1);
            encoder->BindPipelineTextureLayerResource(1, m_TestTexture.get(), 0, i + 1);
            encoder->BindPipelineTextureLayerResource(2, m_BloomBufferTexture.get(), 0, i);
            encoder->BindPipelineTextureLayerResource(3, m_TestTexture.get(), 0, i);
            encoder->FlushPipelineBindings();
            encoder->Dispatch((testData.DstResolution.x / 16) + 1, (testData.DstResolution.y / 16) + 1, 1);
            encoder->EndEncoding();
        }

        m_RenderBuffers.push_back({ m_TestCommandBuffer.get() });
    }

    void SceneRenderer::FinalComposite()
    {
        auto encoder = m_FinalCommandBuffer->EncodeComputeCommands(m_FinalComputeTarget.get());
        encoder->BindPipeline(m_FinalComputePipeline.get());
        encoder->BindPipelineBufferResource(0, m_FrameDataBuffer.get(), 0, 0, 1);
        encoder->BindPipelineTextureResource(1, m_FinalTexture.get());
        encoder->BindPipelineTextureResource(2, m_PreBloomTexture.get());
        encoder->BindPipelineTextureResource(3, m_TestTexture.get());
        encoder->FlushPipelineBindings();
        encoder->Dispatch((m_FinalTexture->GetWidth() / 16) + 1, (m_FinalTexture->GetHeight() / 16) + 1, 1);
        encoder->EndEncoding();

        m_RenderBuffers.push_back({ m_FinalCommandBuffer.get() });
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
}