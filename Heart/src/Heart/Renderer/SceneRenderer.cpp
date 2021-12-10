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
        m_DefaultEnvironmentMap = Texture::Create({ 512, 512, 4, false, 6, 5 });

        // Initialize data buffers
        BufferLayout frameDataLayout = {
            { BufferDataType::Mat4 }, // proj matrix
            { BufferDataType::Mat4 }, // view matrix
            { BufferDataType::Float4 }, // camera pos
            { BufferDataType::Float2 }, // screen size
            { BufferDataType::Bool }, // reverse depth
            { BufferDataType::Float }, // padding
        };
        BufferLayout bloomDataLayout = {
            { BufferDataType::UInt }, // mip level
            { BufferDataType::Bool }, // reverse depth
            { BufferDataType::Float }, // blur scale
            { BufferDataType::Float } // blur strength
        };
        BufferLayout objectDataLayout = {
            { BufferDataType::Mat4 }, // transform
            { BufferDataType::Float4 } // [0]: entityId
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
            { BufferDataType::UInt } // first instance
        };
        m_FrameDataBuffer = Buffer::Create(Buffer::Type::Uniform, BufferUsageType::Dynamic, frameDataLayout, 1, nullptr);
        m_BloomDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, bloomDataLayout, 10, nullptr);
        m_ObjectDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, objectDataLayout, 5000, nullptr);
        m_MaterialDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, materialDataLayout, 5000, nullptr);
        m_LightingDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, lightingDataLayout, 500, nullptr);
        m_IndirectBuffer = Buffer::Create(Buffer::Type::Indirect, BufferUsageType::Dynamic, indirectDataLayout, 1000, nullptr);
        InitializeGridBuffers();

        CreateTextures();

        CreateFramebuffers();
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

        m_PreBloomTexture = Texture::Create({ m_RenderWidth, m_RenderHeight, 4, true, 1, 1, samplerState });
        m_BrightColorsTexture = Texture::Create({ m_RenderWidth, m_RenderHeight, 4, true, 1, m_BloomMipCount, samplerState });
        m_BloomBufferTexture = Texture::Create({ m_RenderWidth, m_RenderHeight, 4, true, 1, m_BloomMipCount - 1, samplerState });
        m_FinalTexture = Texture::Create({ m_RenderWidth, m_RenderHeight, 4, true, 1, 1, samplerState });
    }

    void SceneRenderer::CleanupTextures()
    {
        m_PreBloomTexture.reset();
        m_BrightColorsTexture.reset();
        m_BloomBufferTexture.reset();
        m_FinalTexture.reset();
    }

    void SceneRenderer::CreateFramebuffers()
    {
        // Create the main framebuffer
        FramebufferCreateInfo fbCreateInfo = {
            {
                { false, { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::RGBA8 }, // final [0]
                { true, { -1.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::R32F }, // entity id [1]
                { false, { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::RGBA16F }, // transparency data [2]
                { false, { 1.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::R16F }, // transparency data [3]
                { false, { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::None, m_PreBloomTexture }, // pre-bloom target [4]
                { false, { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::None, m_BrightColorsTexture }, // bright colors target [5]
            },
            {
                {}
            },
            {
                { {}, { { SubpassAttachmentType::Color, 4 }, { SubpassAttachmentType::Color, 5 } } }, // environment map
                { {}, { { SubpassAttachmentType::Color, 4 } } }, // grid
                { {}, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 4 }, { SubpassAttachmentType::Color, 5 }, { SubpassAttachmentType::Color, 1 } } }, // opaque
                { {}, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 1 }, { SubpassAttachmentType::Color, 2 }, { SubpassAttachmentType::Color, 3 } } }, // transparent color
                { { { SubpassAttachmentType::Color, 2 }, { SubpassAttachmentType::Color, 3 } }, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 4 } } }, // composite
            },
            m_RenderWidth, m_RenderHeight,
            MsaaSampleCount::None
        };
        m_FinalFramebuffer = Framebuffer::Create(fbCreateInfo);

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
            { { true, BlendFactor::OneMinusSrcAlpha, BlendFactor::SrcAlpha, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha } },
            true,
            false,
            CullMode::None,
            WindingOrder::Clockwise,
            4
        };
        
        m_FinalFramebuffer->RegisterGraphicsPipeline("skybox", envMapPipeline);
        m_FinalFramebuffer->RegisterGraphicsPipeline("grid", gridPipeline);
        m_FinalFramebuffer->RegisterGraphicsPipeline("pbr", pbrPipeline);
        m_FinalFramebuffer->RegisterGraphicsPipeline("pbrTpColor", transparencyColorPipeline);
        m_FinalFramebuffer->RegisterGraphicsPipeline("tpComposite", transparencyCompositePipeline);

        // Create the bloom framebuffers
        FramebufferCreateInfo bloomFbCreateInfo = {
            {
                { false, { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::None, m_BloomBufferTexture, 0,  },
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
        GraphicsPipelineCreateInfo bloomVertical = {
            AssetManager::GetAssetUUID("Bloom.vert", true),
            AssetManager::GetAssetUUID("BloomVertical.frag", true),
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
        GraphicsPipelineCreateInfo bloomVerticalComposite = {
            AssetManager::GetAssetUUID("Bloom.vert", true),
            AssetManager::GetAssetUUID("BloomVerticalComposite.frag", true),
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

        // Start at the second lowest mip level
        for (int i = m_BloomMipCount - 2; i >= 0; i--)
        {
            // Output will be same size as mip level
            bloomFbCreateInfo.Width = static_cast<u32>(m_BrightColorsTexture->GetWidth() * pow(0.5f, i));
            bloomFbCreateInfo.Height = static_cast<u32>(m_BrightColorsTexture->GetHeight() * pow(0.5f, i));

            // Write to the same mip level in the bloom buffer but read from one level below
            bloomFbCreateInfo.ColorAttachments[0].MipLevel = i; 

            // Always output to the buffer texture
            bloomFbCreateInfo.ColorAttachments[0].Texture = m_BloomBufferTexture;

            auto horizontal = Framebuffer::Create(bloomFbCreateInfo);
            horizontal->RegisterGraphicsPipeline("bloomHorizontal", bloomHorizontal);

            if (i == 0) // If we are on the last iteration, output directly to the output texture
            {
                bloomFbCreateInfo.ColorAttachments[0].Texture = m_FinalTexture;
                bloomFbCreateInfo.SampleCount = MsaaSampleCount::None;
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
        m_FinalFramebuffer.reset();
        for (auto& bufs : m_BloomFramebuffers)
        {
            bufs[0].reset();
            bufs[1].reset();
        }
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
        m_EnvironmentMap = scene->GetEnvironmentMap();
        m_IndirectBatches.clear();
        m_DeferredIndirectBatches.clear();
        for (auto& list : m_EntityListPool)
            list.clear();

        // Set the global data for this frame
        FrameData frameData = {
            camera.GetProjectionMatrix(), camera.GetViewMatrix(), glm::vec4(cameraPosition, 1.f),
            m_FinalFramebuffer->GetSize(),
            Renderer::IsUsingReverseDepth()
        };
        m_FrameDataBuffer->SetElements(&frameData, 1, 0);

        // Bind the main framebuffer
        m_FinalFramebuffer->Bind();

        // Render the skybox if set
        if (m_EnvironmentMap)
            RenderEnvironmentMap();

        // Draw the grid if set
        m_FinalFramebuffer->StartNextSubpass();
        if (renderSettings.DrawGrid)   
            RenderGrid();

        // Update the light buffer with lights  (TODO: that are on screen)
        UpdateLightingBuffer();

        // Recalculate the indirect render batches
        CalculateBatches();

        // Batches pass
        m_FinalFramebuffer->StartNextSubpass();
        RenderBatches();

        // Composite pass
        m_FinalFramebuffer->StartNextSubpass();
        Composite();

        // Create the mipmaps of the bright colors output for bloom
        m_BrightColorsTexture->RegenerateMipMapsSync(m_FinalFramebuffer.get());

        // Submit the framebuffer
        Renderer::Api().RenderFramebuffers(context, { m_FinalFramebuffer.get() });

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
                u64 hash = mesh.Mesh ^ (i * 123192);
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
                
                // Push the associated entity to the associated vector from the pool
                batch.Count++;
                m_EntityListPool[batch.EntityListIndex].emplace_back(static_cast<u32>(entity));
            }
        }

        // Loop over the calculated batches and populate the indirect buffer with draw commands. Because we are instancing, we need to make sure each object/material data
        // element gets placed contiguously for each indirect draw call. At this stage, Batch.First is the index of the indirect draw command in the buffer and
        // Batch.Count will equal 1 because it represents how many draw commands are in each batch
        u32 commandIndex = 0;
        u32 renderId = 0;
        for (auto& pair : m_IndirectBatches)
        {
            // Update the draw command index
            pair.second.First = commandIndex;

            // Popupate the indirect buffer
            IndexedIndirectCommand command = {
                pair.second.Mesh->GetIndexBuffer()->GetAllocatedCount(),
                pair.second.Count, 0, 0, renderId
            };
            m_IndirectBuffer->SetElements(&command, 1, commandIndex);
            commandIndex++;

            // Contiguiously set the instance data for each entity associated with this batch
            auto& entityList = m_EntityListPool[pair.second.EntityListIndex];
            for (auto& entity : entityList)
            {
                // Object data
                ObjectData objectData = { m_Scene->GetEntityCachedTransform({ m_Scene, entity }), { entity, 0.f, 0.f, 0.f } };
                m_ObjectDataBuffer->SetElements(&objectData, 1, renderId);

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

                    m_MaterialDataBuffer->SetElements(&materialData, 1, renderId);
                }
                else
                    m_MaterialDataBuffer->SetElements(&AssetManager::RetrieveAsset<MaterialAsset>("DefaultMaterial.hemat", true)->GetMaterial().GetMaterialData(), 1, renderId);

                renderId++;
            }

            // Change the count to represent the number of draw commands
            pair.second.Count = 1;
        }
    }

    void SceneRenderer::RenderEnvironmentMap()
    {
        m_FinalFramebuffer->BindPipeline("skybox");
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, 1, m_FrameDataBuffer.get());
        m_FinalFramebuffer->BindShaderTextureResource(1, m_EnvironmentMap->GetEnvironmentCubemap());

        auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("DefaultCube.gltf", true);
        auto& meshData = meshAsset->GetSubmesh(0);

        m_FinalFramebuffer->FlushBindings();

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
        m_FinalFramebuffer->BindPipeline("grid");

        // Bind frame data
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, 1, m_FrameDataBuffer.get());

        m_FinalFramebuffer->FlushBindings();

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
            m_FinalFramebuffer->BindShaderTextureResource(4, albedoAsset->GetTexture());
        }
        
        if (materialData.HasMetallicRoughness())
        {
            auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetMetallicRoughnessTexture());
            m_FinalFramebuffer->BindShaderTextureResource(5, metallicRoughnessAsset->GetTexture());
        }

        if (materialData.HasNormal())
        {
            auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetNormalTexture());
            m_FinalFramebuffer->BindShaderTextureResource(6, normalAsset->GetTexture());
        }

        if (materialData.HasEmissive())
        {
            auto emissiveAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetEmissiveTexture());
            m_FinalFramebuffer->BindShaderTextureResource(7, emissiveAsset->GetTexture());
        }

        if (materialData.HasOcclusion())
        {
            auto occlusionAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetOcclusionTexture());
            m_FinalFramebuffer->BindShaderTextureResource(8, occlusionAsset->GetTexture());
        }
    }

    void SceneRenderer::BindPBRDefaults()
    {
        // Bind frame data
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, 1, m_FrameDataBuffer.get());

        // Bind object data
        m_FinalFramebuffer->BindShaderBufferResource(1, 0, m_ObjectDataBuffer->GetAllocatedCount(), m_ObjectDataBuffer.get());

        // Bind material data
        m_FinalFramebuffer->BindShaderBufferResource(2, 0, m_MaterialDataBuffer->GetAllocatedCount(), m_MaterialDataBuffer.get());
        
        // Bind lighting data
        m_FinalFramebuffer->BindShaderBufferResource(3, 0, m_LightingDataBuffer->GetAllocatedCount(), m_LightingDataBuffer.get());

        // Default texture binds
        m_FinalFramebuffer->BindShaderTextureResource(4, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(5, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(6, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(7, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(8, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        if (m_EnvironmentMap)
        {
            m_FinalFramebuffer->BindShaderTextureResource(9, m_EnvironmentMap->GetIrradianceCubemap());
            m_FinalFramebuffer->BindShaderTextureResource(10, m_EnvironmentMap->GetPrefilterCubemap());
            m_FinalFramebuffer->BindShaderTextureResource(11, m_EnvironmentMap->GetBRDFTexture());
        }
        else
        {
            m_FinalFramebuffer->BindShaderTextureResource(9, m_DefaultEnvironmentMap.get());
            m_FinalFramebuffer->BindShaderTextureResource(10, m_DefaultEnvironmentMap.get());
            m_FinalFramebuffer->BindShaderTextureLayerResource(11, m_DefaultEnvironmentMap.get(), 0);
        }
    }

    void SceneRenderer::RenderBatches()
    {
        HE_PROFILE_FUNCTION();
        
        // Bind opaque PBR pipeline
        m_FinalFramebuffer->BindPipeline("pbr");

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

            m_FinalFramebuffer->FlushBindings();

            // Draw
            Renderer::Api().BindVertexBuffer(*batch.Mesh->GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*batch.Mesh->GetIndexBuffer());
            Renderer::Api().DrawIndexedIndirect(m_IndirectBuffer.get(), batch.First, batch.Count);
        }

        // Secondary (translucent) batches pass
        m_FinalFramebuffer->StartNextSubpass();
        m_FinalFramebuffer->BindPipeline("pbrTpColor");
        BindPBRDefaults();

        for (auto batch : m_DeferredIndirectBatches)
        {
            if (batch->Material)
                BindMaterial(batch->Material);

            m_FinalFramebuffer->FlushBindings();

            // Draw
            Renderer::Api().BindVertexBuffer(*batch->Mesh->GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*batch->Mesh->GetIndexBuffer());
            Renderer::Api().DrawIndexedIndirect(m_IndirectBuffer.get(), batch->First, batch->Count);
        }
    }

    void SceneRenderer::Composite()
    {
        // Bind alpha compositing pipeline
        m_FinalFramebuffer->BindPipeline("tpComposite");

        // Bind frame data
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, 1, m_FrameDataBuffer.get());

        // Bind the input attachments from the transparent pass
        m_FinalFramebuffer->BindSubpassInputAttachment(1, { SubpassAttachmentType::Color, 2 });
        m_FinalFramebuffer->BindSubpassInputAttachment(2, { SubpassAttachmentType::Color, 3 });

        m_FinalFramebuffer->FlushBindings();

        // Draw the fullscreen triangle
        Renderer::Api().Draw(3, 0, 1);
    }

    void SceneRenderer::Bloom(GraphicsContext& context)
    {
        for (size_t i = 0; i < m_BloomFramebuffers.size(); i++)
        {
            auto& framebuffers = m_BloomFramebuffers[i];

            // Use the framedata buffer to push the lower mip level that we are sampling from 
            BloomData bloomData{};
            bloomData.ReverseDepth = Renderer::IsUsingReverseDepth();
            bloomData.BlurScale = m_SceneRenderSettings.BloomBlurScale;
            bloomData.BlurStrength = m_SceneRenderSettings.BloomBlurStrength;
            bloomData.MipLevel = static_cast<float>(m_BloomFramebuffers.size() - i); // Reverse the index because we start rendering the lowest mip first
            m_BloomDataBuffer->SetElements(&bloomData, 1, i * 2);

            // Horizontal framebuffer
            framebuffers[0]->Bind();
            framebuffers[0]->BindPipeline("bloomHorizontal");
            framebuffers[0]->BindShaderBufferResource(0, i * 2, 1, m_BloomDataBuffer.get());
            framebuffers[0]->BindShaderTextureResource(1, m_BrightColorsTexture.get()); // Read from bright color target
            framebuffers[0]->FlushBindings();
            Renderer::Api().Draw(3, 0, 1);

            // Render
            Renderer::Api().RenderFramebuffers(context, { framebuffers[0].get() });

            // Use the bloomData buffer to push the current mip level that we are sampling from 
            bloomData.MipLevel = static_cast<float>(m_BloomFramebuffers.size() - 1 - i); // Reverse the index because we start rendering the lowest mip first
            m_BloomDataBuffer->SetElements(&bloomData, 1, i * 2 + 1);

            // Vertical framebuffer
            framebuffers[1]->Bind();
            framebuffers[1]->BindPipeline("bloomVertical");
            framebuffers[1]->BindShaderBufferResource(0, i * 2 + 1, 1, m_BloomDataBuffer.get());
            framebuffers[1]->BindShaderTextureResource(1, m_BloomBufferTexture.get());
            if (i == m_BloomFramebuffers.size() - 1)
                framebuffers[1]->BindShaderTextureResource(2, m_PreBloomTexture.get()); // Bind the pre bloom texture if this is the last iteration
            framebuffers[1]->FlushBindings();
            Renderer::Api().Draw(3, 0, 1);

            // Render
            Renderer::Api().RenderFramebuffers(context, { framebuffers[1].get() });
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