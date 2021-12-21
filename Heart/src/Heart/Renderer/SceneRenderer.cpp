#include "hepch.h"
#include "SceneRenderer.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"
#include "Heart/Core/Timing.h"
#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/Texture.h"
#include "Heart/Renderer/Material.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Core/Camera.h"
#include "Heart/Events/AppEvents.h"
#include "Heart/Events/WindowEvents.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Renderer/Pipeline.h"
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
        m_DefaultEnvironmentMap = Texture::Create({ 512, 512, 4, BufferDataType::UInt8, BufferUsageType::Static, 6, 5 });

        // Initialize data buffers
        BufferLayout frameDataLayout = {
            { BufferDataType::Mat4 }, // proj matrix
            { BufferDataType::Mat4 }, // view matrix
            { BufferDataType::Float4 }, // camera pos
            { BufferDataType::Float2 }, // screen size
            { BufferDataType::Bool }, // reverse depth
            { BufferDataType::Float }, // bloom threshold
            { BufferDataType::Bool }, // cull enable
            { BufferDataType::Bool }, // padding
            { BufferDataType::Float2 } // padding
        };
        BufferLayout bloomDataLayout = {
            { BufferDataType::UInt }, // mip level
            { BufferDataType::Bool }, // reverse depth
            { BufferDataType::Float }, // blur scale
            { BufferDataType::Float } // blur strength
        };
        BufferLayout objectDataLayout = {
            { BufferDataType::Mat4 }, // transform
            { BufferDataType::Float4 }, // [0]: entityId
            { BufferDataType::Float4 }, // boundingSphere
        };
        BufferLayout materialDataLayout = {
            { BufferDataType::Float4 }, // position
            { BufferDataType::Float4 }, // emissive factor
            { BufferDataType::Float4 }, // texcoord transform
            { BufferDataType::Float4 }, // has PBR textures
            { BufferDataType::Float4 }, // has textures
            { BufferDataType::Float4 } // scalars
        };
        BufferLayout lightingDataLayout = {
            { BufferDataType::Float4 }, // position
            { BufferDataType::Float4 }, // rotation
            { BufferDataType::Float4 }, // color
            { BufferDataType::Int }, // light type
            { BufferDataType::Float }, // constant attenuation
            { BufferDataType::Float }, // linear attenuation
            { BufferDataType::Float } // quadratic attenuation
        };
        BufferLayout indirectDataLayout = {
            { BufferDataType::UInt }, // index count
            { BufferDataType::UInt }, // instance count
            { BufferDataType::UInt }, // first index
            { BufferDataType::Int }, // vertex offset
            { BufferDataType::UInt }, // first instance
            { BufferDataType::UInt }, // padding
            { BufferDataType::Float2 } // padding
        };
        BufferLayout instanceDataLayout = {
            { BufferDataType::UInt }, // object id
            { BufferDataType::UInt }, // batch id
            { BufferDataType::Float2 }, // padding
        };
        BufferLayout cullDataLayout = {
            { BufferDataType::Float4 }, // frustum planes [0]
            { BufferDataType::Float4 }, // frustum planes [1]
            { BufferDataType::Float4 }, // frustum planes [2]
            { BufferDataType::Float4 }, // frustum planes [3]
            { BufferDataType::Float4 }, // frustum planes [4]
            { BufferDataType::Float4 }, // frustum planes [5]
            { BufferDataType::Float4 } // [0]: drawCount
        };

        u32 maxObjects = 10000;
        m_FrameDataBuffer = Buffer::Create(Buffer::Type::Uniform, BufferUsageType::Dynamic, frameDataLayout, 1, nullptr);
        m_BloomDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, bloomDataLayout, 50, nullptr);
        m_ObjectDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, objectDataLayout, maxObjects, nullptr);
        m_MaterialDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, materialDataLayout, maxObjects, nullptr);
        m_LightingDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, lightingDataLayout, 500, nullptr);
        m_CullDataBuffer = Buffer::Create(Buffer::Type::Uniform, BufferUsageType::Dynamic, cullDataLayout, 1, nullptr);
        m_InstanceDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, instanceDataLayout, maxObjects, nullptr);
        m_FinalInstanceBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, { BufferDataType::Float4 }, maxObjects, nullptr);
        m_IndirectBuffer = Buffer::Create(Buffer::Type::Indirect, BufferUsageType::Dynamic, indirectDataLayout, maxObjects, nullptr);
        InitializeGridBuffers();

        CreateTextures();

        CreateFramebuffers();

        // Create compute pipelines
        ComputePipelineCreateInfo compCreate = {
            AssetManager::GetAssetUUID("IndirectCull.comp", true),
            true
        };
        m_ComputeCullPipeline = ComputePipeline::Create(compCreate);
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
        TextureSamplerState samplerState;
        samplerState.UVWWrap = { SamplerWrapMode::ClampToBorder, SamplerWrapMode::ClampToBorder, SamplerWrapMode::ClampToBorder };

        m_PreBloomTexture = Texture::Create({ m_RenderWidth, m_RenderHeight, 4, BufferDataType::HalfFloat, BufferUsageType::Dynamic, 1, 1, false, samplerState });
        m_BrightColorsTexture = Texture::Create({ m_RenderWidth, m_RenderHeight, 4, BufferDataType::HalfFloat, BufferUsageType::Dynamic, 1, m_BloomMipCount, false, samplerState });
        m_BloomBufferTexture = Texture::Create({ m_RenderWidth, m_RenderHeight, 4, BufferDataType::HalfFloat, BufferUsageType::Dynamic, 1, m_BloomMipCount, false, samplerState });
        m_BloomUpsampleBufferTexture = Texture::Create({ m_RenderWidth, m_RenderHeight, 4, BufferDataType::HalfFloat, BufferUsageType::Dynamic, 1, m_BloomMipCount - 1, false, samplerState });

        m_FinalTexture = Texture::Create({ m_RenderWidth, m_RenderHeight, 4, BufferDataType::UInt8, BufferUsageType::Dynamic, 1, 1, false, samplerState });
        m_EntityIdsTexture = Texture::Create({ m_RenderWidth, m_RenderHeight, 1, BufferDataType::Float, BufferUsageType::Dynamic, 1, 1, true, samplerState });
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
        // Create the main framebuffer
        FramebufferCreateInfo fbCreateInfo = {
            {
                { { -1.f, 0.f, 0.f, 0.f }, true, Heart::ColorFormat::R32F, m_EntityIdsTexture }, // entity id [0]
                { { 0.f, 0.f, 0.f, 0.f }, false, Heart::ColorFormat::RGBA16F }, // transparency data [1]
                { { 1.f, 0.f, 0.f, 0.f }, false, Heart::ColorFormat::R16F }, // transparency data [2]
                { { 0.f, 0.f, 0.f, 0.f }, false, Heart::ColorFormat::RGBA16F, m_PreBloomTexture }, // pre-bloom target [3]
                { { 0.f, 0.f, 0.f, 0.f }, false, Heart::ColorFormat::RGBA16F, m_BrightColorsTexture }, // bright colors target [4]
            },
            {
                {}
            },
            {
                // TODO: transparent pass bloom contribution
                { {}, { { SubpassAttachmentType::Color, 3 }, { SubpassAttachmentType::Color, 4 } } }, // environment map
                { {}, { { SubpassAttachmentType::Color, 3 } } }, // grid
                { {}, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 3 }, { SubpassAttachmentType::Color, 4 }, { SubpassAttachmentType::Color, 0 } } }, // opaque
                { {}, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 0 }, { SubpassAttachmentType::Color, 1 }, { SubpassAttachmentType::Color, 2 } } }, // transparent color
                { { { SubpassAttachmentType::Color, 1 }, { SubpassAttachmentType::Color, 2 } }, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 3 }, { SubpassAttachmentType::Color, 4 } } }, // composite
            },
            m_RenderWidth, m_RenderHeight,
            MsaaSampleCount::None,
            true
        };
        m_MainFramebuffer = Framebuffer::Create(fbCreateInfo);

        // Register pipelines
        GraphicsPipelineCreateInfo envMapPipeline = {
            AssetManager::GetAssetUUID("Skybox.vert", true),
            AssetManager::GetAssetUUID("Skybox.frag", true),
            true,
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { false }, { false } },
            false,
            false,
            CullMode::None,
            WindingOrder::Clockwise,
            0
        };
        GraphicsPipelineCreateInfo gridPipeline = {
            AssetManager::GetAssetUUID("Grid.vert", true),
            AssetManager::GetAssetUUID("Grid.frag", true),
            true,
            VertexTopology::LineList,
            { BufferDataType::Float3 },
            { { false } },
            false,
            false,
            CullMode::None,
            WindingOrder::Clockwise,
            1
        };
        GraphicsPipelineCreateInfo pbrPipeline = {
            AssetManager::GetAssetUUID("PBR.vert", true),
            AssetManager::GetAssetUUID("PBR.frag", true),
            true,
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { false }, { false }, { false } },
            true,
            true,
            CullMode::Backface,
            WindingOrder::Clockwise,
            2
        };
        GraphicsPipelineCreateInfo transparencyColorPipeline = {
            AssetManager::GetAssetUUID("PBR.vert", true),
            AssetManager::GetAssetUUID("PBRTransparentColor.frag", true),
            true,
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { false }, { true, BlendFactor::One, BlendFactor::One, BlendFactor::One, BlendFactor::One }, { true, BlendFactor::Zero, BlendFactor::OneMinusSrcColor, BlendFactor::Zero, BlendFactor::OneMinusSrcAlpha } },
            true,
            true,
            CullMode::None,
            WindingOrder::Clockwise,
            3
        };
        GraphicsPipelineCreateInfo transparencyCompositePipeline = {
            AssetManager::GetAssetUUID("FullscreenTriangle.vert", true),
            AssetManager::GetAssetUUID("TransparentComposite.frag", true),
            false,
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { true, BlendFactor::OneMinusSrcAlpha, BlendFactor::SrcAlpha, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha }, { false } },
            true,
            false,
            CullMode::None,
            WindingOrder::Clockwise,
            4
        };
        
        m_MainFramebuffer->RegisterGraphicsPipeline("skybox", envMapPipeline);
        m_MainFramebuffer->RegisterGraphicsPipeline("grid", gridPipeline);
        m_MainFramebuffer->RegisterGraphicsPipeline("pbr", pbrPipeline);
        m_MainFramebuffer->RegisterGraphicsPipeline("pbrTpColor", transparencyColorPipeline);
        m_MainFramebuffer->RegisterGraphicsPipeline("tpComposite", transparencyCompositePipeline);

        // Create the bloom framebuffers
        FramebufferCreateInfo bloomFbCreateInfo = {
            {
                { { 0.f, 0.f, 0.f, 0.f }, false, Heart::ColorFormat::None, m_BloomBufferTexture, 0, 0 },
            },
            {
                {}
            },
            {
                { {}, { { SubpassAttachmentType::Color, 0 } } }
            },
            m_RenderWidth, m_RenderHeight,
            MsaaSampleCount::None
        };

        GraphicsPipelineCreateInfo bloomHorizontal = {
            AssetManager::GetAssetUUID("Bloom.vert", true),
            AssetManager::GetAssetUUID("BloomHorizontal.frag", true),
            false,
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { false } },
            false,
            false,
            CullMode::None,
            WindingOrder::Clockwise,
            0
        };
        
        GraphicsPipelineCreateInfo bloomHorizontalUpscale = bloomHorizontal;
        bloomHorizontalUpscale.FragmentShaderAsset = AssetManager::GetAssetUUID("BloomHorizontalUpscale.frag", true);
        bloomHorizontalUpscale.BlendStates.push_back({ false });

        GraphicsPipelineCreateInfo bloomHorizontalDoubleUpscale = bloomHorizontal;
        bloomHorizontalDoubleUpscale.FragmentShaderAsset = AssetManager::GetAssetUUID("BloomHorizontalDoubleUpscale.frag", true);
        bloomHorizontalDoubleUpscale.BlendStates.push_back({ false });

        GraphicsPipelineCreateInfo bloomVertical = bloomHorizontal;
        bloomVertical.FragmentShaderAsset = AssetManager::GetAssetUUID("BloomVertical.frag", true);

        GraphicsPipelineCreateInfo bloomVerticalComposite = bloomHorizontal;
        bloomVerticalComposite.FragmentShaderAsset = AssetManager::GetAssetUUID("BloomVerticalComposite.frag", true);

        // Start at the lowest mip level
        for (int i = m_BloomMipCount - 1; i >= 0; i--)
        {
            // Output will be same size as mip level
            bloomFbCreateInfo.Width = static_cast<u32>(m_BrightColorsTexture->GetWidth() * pow(0.5f, i));
            bloomFbCreateInfo.Height = static_cast<u32>(m_BrightColorsTexture->GetHeight() * pow(0.5f, i));

            // Write to the same mip level in the bloom buffer but read from one level below
            bloomFbCreateInfo.ColorAttachments[0].MipLevel = i; 

            // Output to the buffer texture
            bloomFbCreateInfo.ColorAttachments[0].Texture = m_BloomBufferTexture;
            if (i < m_BloomMipCount - 1)
            {
                // Starting after the bottom mip level, push back the second color attachment which will be the upsample buffer
                bloomFbCreateInfo.ColorAttachments.push_back(
                    { { 0.f, 0.f, 0.f, 0.f }, false, Heart::ColorFormat::None, m_BloomUpsampleBufferTexture, 0, static_cast<u32>(i) }
                );
                bloomFbCreateInfo.Subpasses[0].OutputAttachments.push_back(
                    { SubpassAttachmentType::Color, 1 }
                );
            }

            auto horizontal = Framebuffer::Create(bloomFbCreateInfo);
            if (i == m_BloomMipCount - 1) // If we are on the first iteration, we want to run the basic horizontal shader
                horizontal->RegisterGraphicsPipeline("bloomHorizontal", bloomHorizontal);
            else if (i == m_BloomMipCount - 2) // If we are on the second iteration, we want to run the bright color upscale shader
                horizontal->RegisterGraphicsPipeline("bloomHorizontal", bloomHorizontalUpscale);
            else // Otherwise we want to run the shader that upscales both the bright color and the upsample buffer
                horizontal->RegisterGraphicsPipeline("bloomHorizontal", bloomHorizontalDoubleUpscale);

            // Get rid of the second color attachment for the vertical pass
            bloomFbCreateInfo.ColorAttachments.resize(1);
            bloomFbCreateInfo.Subpasses[0].OutputAttachments.resize(1);

            if (i == 0) // If we are on the last iteration, output directly to the output texture
            {
                bloomFbCreateInfo.ColorAttachments[0].Texture = m_FinalTexture;
                bloomFbCreateInfo.SampleCount = MsaaSampleCount::Four;
            }
            else // Otherwise we are outputting to the bright color texture
                bloomFbCreateInfo.ColorAttachments[0].Texture = m_BrightColorsTexture;

            auto vertical = Framebuffer::Create(bloomFbCreateInfo);
            if (i == 0) // If we are on the last iteration, we want to run the composite version of the blur shader
                vertical->RegisterGraphicsPipeline("bloomVertical", bloomVerticalComposite);
            else
                vertical->RegisterGraphicsPipeline("bloomVertical", bloomVertical);

            m_BloomFramebuffers.push_back({ horizontal, vertical });
        }
    }

    void SceneRenderer::CleanupFramebuffers()
    {
        m_MainFramebuffer.reset();
        m_BloomFramebuffers.clear();
    }

    void SceneRenderer::RenderScene(GraphicsContext& context, Scene* scene, const Camera& camera, glm::vec3 cameraPosition, const SceneRenderSettings& renderSettings)
    {
        HE_PROFILE_FUNCTION();
        auto timer = Heart::AggregateTimer("SceneRenderer::RenderScene");

        HE_ENGINE_ASSERT(scene, "Scene cannot be nullptr");

        if (m_ShouldResize)
            Resize();

        // Reset in-flight frame data
        m_SceneRenderSettings = renderSettings;
        m_Scene = scene;
        m_Camera = &camera;
        m_EnvironmentMap = scene->GetEnvironmentMap();
        m_IndirectBatches.clear();
        m_DeferredIndirectBatches.clear();
        for (auto& list : m_EntityListPool)
            list.clear();

        // Set the global data for this frame
        FrameData frameData = {
            camera.GetProjectionMatrix(), camera.GetViewMatrix(), glm::vec4(cameraPosition, 1.f),
            m_MainFramebuffer->GetSize(),
            Renderer::IsUsingReverseDepth(),
            m_SceneRenderSettings.BloomThreshold,
            m_SceneRenderSettings.CullEnable
        };
        m_FrameDataBuffer->SetElements(&frameData, 1, 0);

        // Update the light buffer with lights  (TODO: that are on screen)
        UpdateLightingBuffer();

        // Recalculate the indirect render batches
        CalculateBatches();

        // Run the cull shader if enabled
        if (m_SceneRenderSettings.CullEnable)
        {
            SetupCullCompute();
            m_MainFramebuffer->Bind(m_ComputeCullPipeline.get());
        }
        else
            m_MainFramebuffer->Bind();

        // Render the skybox if set
        if (m_EnvironmentMap)
            RenderEnvironmentMap();

        // Draw the grid if set
        m_MainFramebuffer->StartNextSubpass();
        if (renderSettings.DrawGrid)   
            RenderGrid();

        // Batches pass
        m_MainFramebuffer->StartNextSubpass();
        RenderBatches();

        // Composite pass
        m_MainFramebuffer->StartNextSubpass();
        Composite();

        // Create the mipmaps of the bright colors output for bloom
        if (m_SceneRenderSettings.BloomEnable)
            m_BrightColorsTexture->RegenerateMipMapsSync(m_MainFramebuffer.get());

        // Submit the framebuffer
        Renderer::Api().RenderFramebuffers(context, { { m_MainFramebuffer.get() } });

        // Bloom
        Bloom(context);
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
            m_LightingDataBuffer->SetBytes(&entity.GetWorldPosition(), sizeof(glm::vec3), offset);
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

        // Loop over each mesh component / submesh, hash the mesh & material, and place the entity in a batch
        // associated with the mesh & material. At this stage, Batch.First is unused and Batch.Count indicates
        // how many instances there are
        u32 batchIndex = 0;
        auto group = m_Scene->GetRegistry().group<TransformComponent, MeshComponent>();
        for (auto entity : group)
        {
            auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

            // Skip invalid meshes
            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(mesh.Mesh);
            if (!meshAsset || !meshAsset->IsValid()) continue;

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);

                // Create a hash based on the submesh and its material if applicable
                u64 hash = mesh.Mesh ^ (i * 45787893);
                if (meshData.GetMaterialIndex() < mesh.Materials.size())
                    hash ^= mesh.Materials[meshData.GetMaterialIndex()];

                // Get/create a batch associated with this hash
                auto& batch = m_IndirectBatches[hash];

                // Update the batch information if this is the first entity being added to it
                if (batch.Count == 0)
                {
                    // Retrieve a vector from the pool
                    batch.EntityListIndex = batchIndex;
                    if (batchIndex >= m_EntityListPool.size())
                        m_EntityListPool.emplace_back();

                    // Set the material & mesh associated with this batch
                    batch.Mesh = &meshData;
                    batch.Material = &meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()]; // default material
                    if (mesh.Materials.size() > meshData.GetMaterialIndex())
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
                m_EntityListPool[batch.EntityListIndex].emplace_back(static_cast<u32>(entity));
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

                    auto albedoAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetAlbedoTexture());
                    materialData.SetHasAlbedo(albedoAsset && albedoAsset->IsValid());

                    auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetMetallicRoughnessTexture());
                    materialData.SetHasMetallicRoughness(metallicRoughnessAsset && metallicRoughnessAsset->IsValid());

                    auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetNormalTexture());
                    materialData.SetHasNormal(normalAsset && normalAsset->IsValid());

                    auto emissiveAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetEmissiveTexture());
                    materialData.SetHasEmissive(emissiveAsset && emissiveAsset->IsValid());

                    auto occlusionAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetOcclusionTexture());
                    materialData.SetHasOcclusion(occlusionAsset && occlusionAsset->IsValid());

                    m_MaterialDataBuffer->SetElements(&materialData, 1, objectId);
                }
                else
                    m_MaterialDataBuffer->SetElements(&AssetManager::RetrieveAsset<MaterialAsset>("DefaultMaterial.hemat", true)->GetMaterial().GetMaterialData(), 1, objectId);

                objectId++;
            }

            // Change the count to represent the number of draw commands
            pair.second.Count = 1;

            commandIndex++;
        }
    }

    void SceneRenderer::SetupCullCompute()
    {
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
    }

    void SceneRenderer::RenderEnvironmentMap()
    {
        m_MainFramebuffer->BindPipeline("skybox");
        m_MainFramebuffer->BindShaderBufferResource(0, 0, 1, m_FrameDataBuffer.get());
        m_MainFramebuffer->BindShaderTextureResource(1, m_EnvironmentMap->GetEnvironmentCubemap());

        auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("DefaultCube.gltf", true);
        auto& meshData = meshAsset->GetSubmesh(0);

        m_MainFramebuffer->FlushBindings();

        Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
        Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
        Renderer::Api().DrawIndexed(
            meshData.GetIndexBuffer()->GetAllocatedCount(),
            0, 0, 1
        );
    }

    void SceneRenderer::RenderGrid()
    {
        // Bind grid pipeline
        m_MainFramebuffer->BindPipeline("grid");

        // Bind frame data
        m_MainFramebuffer->BindShaderBufferResource(0, 0, 1, m_FrameDataBuffer.get());

        m_MainFramebuffer->FlushBindings();

        // Draw
        Renderer::Api().SetLineWidth(2.f);
        Renderer::Api().BindVertexBuffer(*m_GridVertices);
        Renderer::Api().BindIndexBuffer(*m_GridIndices);
        Renderer::Api().DrawIndexed(
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
            m_MainFramebuffer->BindShaderTextureResource(4, albedoAsset->GetTexture());
        }
        
        if (materialData.HasMetallicRoughness())
        {
            auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetMetallicRoughnessTexture());
            m_MainFramebuffer->BindShaderTextureResource(5, metallicRoughnessAsset->GetTexture());
        }

        if (materialData.HasNormal())
        {
            auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetNormalTexture());
            m_MainFramebuffer->BindShaderTextureResource(6, normalAsset->GetTexture());
        }

        if (materialData.HasEmissive())
        {
            auto emissiveAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetEmissiveTexture());
            m_MainFramebuffer->BindShaderTextureResource(7, emissiveAsset->GetTexture());
        }

        if (materialData.HasOcclusion())
        {
            auto occlusionAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetOcclusionTexture());
            m_MainFramebuffer->BindShaderTextureResource(8, occlusionAsset->GetTexture());
        }
    }

    void SceneRenderer::BindPBRDefaults()
    {
        // Bind frame data
        m_MainFramebuffer->BindShaderBufferResource(0, 0, 1, m_FrameDataBuffer.get());

        // Bind object data
        m_MainFramebuffer->BindShaderBufferResource(1, 0, m_ObjectDataBuffer->GetAllocatedCount(), m_ObjectDataBuffer.get());

        // Bind culled instance map data
        m_MainFramebuffer->BindShaderBufferResource(12, 0, m_FinalInstanceBuffer->GetAllocatedCount(), m_FinalInstanceBuffer.get());

        // Bind material data
        m_MainFramebuffer->BindShaderBufferResource(2, 0, m_MaterialDataBuffer->GetAllocatedCount(), m_MaterialDataBuffer.get());
        
        // Bind lighting data
        m_MainFramebuffer->BindShaderBufferResource(3, 0, m_LightingDataBuffer->GetAllocatedCount(), m_LightingDataBuffer.get());

        // Default texture binds
        m_MainFramebuffer->BindShaderTextureResource(4, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_MainFramebuffer->BindShaderTextureResource(5, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_MainFramebuffer->BindShaderTextureResource(6, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_MainFramebuffer->BindShaderTextureResource(7, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_MainFramebuffer->BindShaderTextureResource(8, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        if (m_EnvironmentMap)
        {
            m_MainFramebuffer->BindShaderTextureResource(9, m_EnvironmentMap->GetIrradianceCubemap());
            m_MainFramebuffer->BindShaderTextureResource(10, m_EnvironmentMap->GetPrefilterCubemap());
            m_MainFramebuffer->BindShaderTextureResource(11, m_EnvironmentMap->GetBRDFTexture());
        }
        else
        {
            m_MainFramebuffer->BindShaderTextureResource(9, m_DefaultEnvironmentMap.get());
            m_MainFramebuffer->BindShaderTextureResource(10, m_DefaultEnvironmentMap.get());
            m_MainFramebuffer->BindShaderTextureLayerResource(11, m_DefaultEnvironmentMap.get(), 0, 0);
        }
    }

    void SceneRenderer::RenderBatches()
    {
        HE_PROFILE_FUNCTION();
        
        // Bind opaque PBR pipeline
        m_MainFramebuffer->BindPipeline("pbr");

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
                    m_DeferredIndirectBatches.emplace_back(&batch);
                    continue;
                }
                BindMaterial(batch.Material);
            }

            m_MainFramebuffer->FlushBindings();

            // Draw
            Renderer::Api().BindVertexBuffer(*batch.Mesh->GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*batch.Mesh->GetIndexBuffer());
            Renderer::Api().DrawIndexedIndirect(m_IndirectBuffer.get(), batch.First, batch.Count);
        }

        // Secondary (translucent) batches pass
        m_MainFramebuffer->StartNextSubpass();
        m_MainFramebuffer->BindPipeline("pbrTpColor");
        BindPBRDefaults();

        for (auto batch : m_DeferredIndirectBatches)
        {
            if (batch->Material)
                BindMaterial(batch->Material);

            m_MainFramebuffer->FlushBindings();

            // Draw
            Renderer::Api().BindVertexBuffer(*batch->Mesh->GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*batch->Mesh->GetIndexBuffer());
            Renderer::Api().DrawIndexedIndirect(m_IndirectBuffer.get(), batch->First, batch->Count);
        }
    }

    void SceneRenderer::Composite()
    {
        // Bind alpha compositing pipeline
        m_MainFramebuffer->BindPipeline("tpComposite");

        // Bind frame data
        m_MainFramebuffer->BindShaderBufferResource(0, 0, 1, m_FrameDataBuffer.get());

        // Bind the input attachments from the transparent pass
        m_MainFramebuffer->BindSubpassInputAttachment(1, { SubpassAttachmentType::Color, 1 });
        m_MainFramebuffer->BindSubpassInputAttachment(2, { SubpassAttachmentType::Color, 2 });

        m_MainFramebuffer->FlushBindings();

        // Draw the fullscreen triangle
        Renderer::Api().Draw(3, 0, 1);
    }

    void SceneRenderer::Bloom(GraphicsContext& context)
    {
        if (m_SceneRenderSettings.BloomEnable)
        {
            for (size_t i = 0; i < m_BloomFramebuffers.size(); i++)
            {
                auto& framebuffers = m_BloomFramebuffers[i];

                // Use the bloomData buffer to push the lower mip level that we are sampling from 
                BloomData bloomData{};
                bloomData.ReverseDepth = Renderer::IsUsingReverseDepth();
                bloomData.BlurScale = m_SceneRenderSettings.BloomBlurScale;
                bloomData.BlurStrength = m_SceneRenderSettings.BloomBlurStrength;
                bloomData.MipLevel = static_cast<u32>(m_BloomFramebuffers.size() - 1 - i); // Reverse the index because we start rendering the lowest mip first
                m_BloomDataBuffer->SetElements(&bloomData, 1, i);

                // Horizontal framebuffer
                framebuffers[0]->Bind();
                framebuffers[0]->BindPipeline("bloomHorizontal");
                framebuffers[0]->BindShaderBufferResource(0, i, 1, m_BloomDataBuffer.get());
                framebuffers[0]->BindShaderTextureResource(1, m_BrightColorsTexture.get()); // Read from bright color target
                if (i > 1)
                {
                    // Bind the upsample texture if this is at least the third iteration so we can upsample
                    framebuffers[0]->BindShaderTextureLayerResource(2, m_BloomUpsampleBufferTexture.get(), 0, bloomData.MipLevel + 1);
                }
                framebuffers[0]->FlushBindings();
                Renderer::Api().Draw(3, 0, 1);

                // Render
                Renderer::Api().RenderFramebuffers(context, { { framebuffers[0].get() } });

                // Vertical framebuffer
                framebuffers[1]->Bind();
                framebuffers[1]->BindPipeline("bloomVertical");
                framebuffers[1]->BindShaderBufferResource(0, i, 1, m_BloomDataBuffer.get());
                framebuffers[1]->BindShaderTextureResource(1, m_BloomBufferTexture.get());
                if (i == m_BloomFramebuffers.size() - 1)
                {
                    // Bind the pre bloom & upsample texture if this is the last iteration
                    framebuffers[1]->BindShaderTextureResource(2, m_PreBloomTexture.get());
                    framebuffers[1]->BindShaderTextureResource(3, m_BloomUpsampleBufferTexture.get());
                }
                framebuffers[1]->FlushBindings();
                Renderer::Api().Draw(3, 0, 1);

                // Render
                Renderer::Api().RenderFramebuffers(context, { { framebuffers[1].get() } });
            }
        }
        else
        {
            auto& framebuffers = m_BloomFramebuffers[m_BloomFramebuffers.size() - 1];

            // Clear the horizontal blur texture so that the final composite shader only inputs from the HDR output
            framebuffers[0]->Bind();
            framebuffers[0]->ClearOutputAttachment(0, false);
            Renderer::Api().RenderFramebuffers(context, { { framebuffers[0].get() } });

            framebuffers[1]->Bind();
            framebuffers[1]->BindPipeline("bloomVertical");
            framebuffers[1]->BindShaderBufferResource(0, m_BloomFramebuffers.size() - 1, 1, m_BloomDataBuffer.get());
            framebuffers[1]->BindShaderTextureResource(1, m_BloomBufferTexture.get());
            framebuffers[1]->BindShaderTextureResource(2, m_PreBloomTexture.get());
            framebuffers[1]->BindShaderTextureResource(3, m_BloomUpsampleBufferTexture.get());
            framebuffers[1]->FlushBindings();
            Renderer::Api().Draw(3, 0, 1);

            // Render
            Renderer::Api().RenderFramebuffers(context, { { framebuffers[1].get() } });
        }
    }

    void SceneRenderer::InitializeGridBuffers()
    {
        // Default size (TODO: parameterize)
        u32 gridSize = 20;

        std::vector<glm::vec3> vertices;
        vertices.reserve(static_cast<size_t>(pow(gridSize + 1, 2)));
        std::vector<u32> indices;
        
        // Calculate the grid with a line list
        glm::vec3 pos = { gridSize * -0.5f, 0.f, gridSize * -0.5f };
        u32 vertexIndex = 0;
        for (u32 i = 0; i <= gridSize; i++)
        {
            for (u32 j = 0; j <= gridSize; j++)
            {
                vertices.emplace_back(pos);

                if (j != 0)
                {
                    indices.emplace_back(vertexIndex);
                    indices.emplace_back(vertexIndex - 1);
                }
                if (i != 0)
                {
                    indices.emplace_back(vertexIndex);
                    indices.emplace_back(vertexIndex - gridSize - 1);
                }

                pos.x += 1.f;
                vertexIndex++;
            }
            pos.x = gridSize * -0.5f;
            pos.z += 1.f;
        }

        m_GridVertices = Buffer::Create(Buffer::Type::Vertex, BufferUsageType::Static, { BufferDataType::Float3 }, static_cast<u32>(vertices.size()), (void*)vertices.data());
        m_GridIndices = Buffer::CreateIndexBuffer(BufferUsageType::Static, static_cast<u32>(indices.size()), (void*)indices.data());
    }
}