#include "htpch.h"
#include "SceneRenderer.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Timing.h"
#include "Heart/Renderer/Renderer.h"
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
        Initialize();
    }

    SceneRenderer::~SceneRenderer()
    {
        UnsubscribeFromEmitter(&App::Get());
        Shutdown();
    }

    void SceneRenderer::OnEvent(Event& event)
    {
        event.Map<AppGraphicsInitEvent>(HE_BIND_EVENT_FN(SceneRenderer::OnAppGraphicsInit));
        event.Map<AppGraphicsShutdownEvent>(HE_BIND_EVENT_FN(SceneRenderer::OnAppGraphicsShutdown));
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
            { BufferDataType::Bool } // padding
        };
        BufferLayout objectDataLayout = {
            { BufferDataType::Mat4 }, // transform
            { BufferDataType::Float4 } // [0]: entityId
        };
        BufferLayout materialDataLayout = {
            { BufferDataType::Float4 }, // base color
            { BufferDataType::Float4 }, // emissive factor
            { BufferDataType::Float4 }, // texcoord transform
            { BufferDataType::Float4 }, // has PBR textures
            { BufferDataType::Float4 }, // has textures
            { BufferDataType::Float4 } // scalars
        };
        BufferLayout indirectDataLayout = {
            { BufferDataType::UInt }, // index count
            { BufferDataType::UInt }, // instance count
            { BufferDataType::UInt }, // first index
            { BufferDataType::Int }, // vertex offset
            { BufferDataType::UInt } // first instance
        };
        m_FrameDataBuffer = Buffer::Create(Buffer::Type::Uniform, BufferUsageType::Dynamic, frameDataLayout, 1, nullptr);
        m_ObjectDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, objectDataLayout, 5000, nullptr);
        m_MaterialDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, materialDataLayout, 5000, nullptr);
        m_IndirectBuffer = Buffer::Create(Buffer::Type::Indirect, BufferUsageType::Dynamic, indirectDataLayout, 1000, nullptr);
        InitializeGridBuffers();

        // Create the main framebuffer
        FramebufferCreateInfo fbCreateInfo = {
            {
                { false, { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::RGBA8 },
                { true, { -1.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::R32F },
                { false, { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::RGBA16F },
                { false, { 1.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::R16F }
            },
            {
                {}
            },
            {
                { {}, { { SubpassAttachmentType::Color, 0 } } }, // environment map
                { {}, { { SubpassAttachmentType::Color, 0 } } }, // grid
                { {}, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 0 }, { SubpassAttachmentType::Color, 1 } } }, // opaque
                { {}, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 1 }, { SubpassAttachmentType::Color, 2 }, { SubpassAttachmentType::Color, 3 } } }, // transparent color
                { { { SubpassAttachmentType::Color, 2 }, { SubpassAttachmentType::Color, 3 } }, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 0 } } } // composite
            },
            0, 0,
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
            { { false } },
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
            { { false }, { false } },
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
    }

    void SceneRenderer::Shutdown()
    {
        m_Initialized = false;

        m_DefaultEnvironmentMap.reset();
        m_FinalFramebuffer.reset();
        m_FrameDataBuffer.reset();
        m_ObjectDataBuffer.reset();
        m_MaterialDataBuffer.reset();
        m_IndirectBuffer.reset();

        m_GridVertices.reset();
        m_GridIndices.reset();
    }

    void SceneRenderer::RenderScene(GraphicsContext& context, Scene* scene, const Camera& camera, glm::vec3 cameraPosition, bool drawGrid)
    {
        HE_PROFILE_FUNCTION();
        auto timer = Heart::AggregateTimer("SceneRenderer::RenderScene");

        HE_ENGINE_ASSERT(scene, "Scene cannot be nullptr");

        // Reset in-flight frame data
        m_Scene = scene;
        m_EnvironmentMap = scene->GetEnvironmentMap();
        m_IndirectBatches.clear();
        m_DeferredIndirectBatches.clear();
        for (auto& list : m_EntityListPool)
            list.clear();

        // Set the global data for this frame
        FrameData frameData = { camera.GetProjectionMatrix(), camera.GetViewMatrix(), glm::vec4(cameraPosition, 1.f), m_FinalFramebuffer->GetSize(), Renderer::IsUsingReverseDepth() };
        m_FrameDataBuffer->SetData(&frameData, 1, 0);

        // Bind the main framebuffer
        m_FinalFramebuffer->Bind();

        // Render the skybox if set
        if (m_EnvironmentMap)
            RenderEnvironmentMap();

        // Draw the grid if set
        m_FinalFramebuffer->StartNextSubpass();
        if (drawGrid)   
            RenderGrid();

        // Recalculate the indirect render batches
        CalculateBatches();

        // Batches pass
        m_FinalFramebuffer->StartNextSubpass();
        RenderBatches();

        // Composite pass
        m_FinalFramebuffer->StartNextSubpass();
        Composite();

        // Submit the framebuffer
        Renderer::Api().RenderFramebuffers(context, { m_FinalFramebuffer.get() });
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
            m_IndirectBuffer->SetData(&command, 1, commandIndex);
            commandIndex++;

            // Contiguiously set the instance data for each entity associated with this batch
            auto& entityList = m_EntityListPool[pair.second.EntityListIndex];
            for (auto& entity : entityList)
            {
                // Object data
                ObjectData objectData = { m_Scene->GetEntityCachedTransform({ m_Scene, entity }), { entity, 0.f, 0.f, 0.f } };
                m_ObjectDataBuffer->SetData(&objectData, 1, renderId);

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

                    m_MaterialDataBuffer->SetData(&materialData, 1, renderId);
                }
                else
                    m_MaterialDataBuffer->SetData(&AssetManager::RetrieveAsset<MaterialAsset>("DefaultMaterial.hemat", true)->GetMaterial().GetMaterialData(), 1, renderId);

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
        m_FinalFramebuffer->BindShaderTextureResource(1, m_EnvironmentMap->GetPrefilterCubemap());

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
            m_FinalFramebuffer->BindShaderTextureResource(3, albedoAsset->GetTexture());
        }
        
        if (materialData.HasMetallicRoughness())
        {
            auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetMetallicRoughnessTexture());
            m_FinalFramebuffer->BindShaderTextureResource(4, metallicRoughnessAsset->GetTexture());
        }

        if (materialData.HasNormal())
        {
            auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetNormalTexture());
            m_FinalFramebuffer->BindShaderTextureResource(5, normalAsset->GetTexture());
        }

        if (materialData.HasEmissive())
        {
            auto emissiveAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetEmissiveTexture());
            m_FinalFramebuffer->BindShaderTextureResource(6, emissiveAsset->GetTexture());
        }

        if (materialData.HasOcclusion())
        {
            auto occlusionAsset = AssetManager::RetrieveAsset<TextureAsset>(material->GetOcclusionTexture());
            m_FinalFramebuffer->BindShaderTextureResource(7, occlusionAsset->GetTexture());
        }
    }

    void SceneRenderer::BindPBRDefaults()
    {
        // Bind frame data
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, 1, m_FrameDataBuffer.get());

        // Bind object data
        m_FinalFramebuffer->BindShaderBufferResource(1, 0, m_ObjectDataBuffer->GetAllocatedCount(), m_ObjectDataBuffer.get());
        m_FinalFramebuffer->BindShaderBufferResource(2, 0, m_MaterialDataBuffer->GetAllocatedCount(), m_MaterialDataBuffer.get());

        // Default texture binds
        m_FinalFramebuffer->BindShaderTextureResource(3, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(4, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(5, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(6, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(7, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        if (m_EnvironmentMap)
        {
            m_FinalFramebuffer->BindShaderTextureResource(8, m_EnvironmentMap->GetIrradianceCubemap());
            m_FinalFramebuffer->BindShaderTextureResource(9, m_EnvironmentMap->GetPrefilterCubemap());
            m_FinalFramebuffer->BindShaderTextureResource(10, m_EnvironmentMap->GetBRDFTexture());
        }
        else
        {
            m_FinalFramebuffer->BindShaderTextureResource(8, m_DefaultEnvironmentMap.get());
            m_FinalFramebuffer->BindShaderTextureResource(9, m_DefaultEnvironmentMap.get());
            m_FinalFramebuffer->BindShaderTextureLayerResource(10, m_DefaultEnvironmentMap.get(), 0);
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