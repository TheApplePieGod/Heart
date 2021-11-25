#include "htpch.h"
#include "SceneRenderer.h"

#include "Heart/Core/App.h"
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
            { BufferDataType::Int }, // entityId
            { BufferDataType::Float3 } // padding
        };
        BufferLayout materialDataLayout = {
            { BufferDataType::Float4 }, // base color
            { BufferDataType::Float4 }, // emissive factor
            { BufferDataType::Float4 }, // texcoord transform
            { BufferDataType::Float4 }, // has PBR textures
            { BufferDataType::Float4 }, // has textures
            { BufferDataType::Float4 } // scalars
        };
        m_FrameDataBuffer = Buffer::Create(Buffer::Type::Uniform, BufferUsageType::Dynamic, frameDataLayout, 1, nullptr);
        m_ObjectDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, objectDataLayout, 2000, nullptr);
        m_MaterialDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, materialDataLayout, 2000, nullptr);
        InitializeGridBuffers();

        // Create the main framebuffer
        FramebufferCreateInfo fbCreateInfo = {
            {
                { false, { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::RGBA8 },
                { true, { -1.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::R32F },
                { false, { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::RGBA32F },
                { false, { 1.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::R32F }
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

    SceneRenderer::~SceneRenderer()
    {
        
    }

    void SceneRenderer::RenderScene(GraphicsContext& context, Scene* scene, const Camera& camera, glm::vec3 cameraPosition, bool drawGrid, EnvironmentMap* envMap)
    {
        HE_PROFILE_FUNCTION();
        HE_ENGINE_ASSERT(scene, "Scene cannot be nullptr");

        // Reset in-flight frame data
        m_Scene = scene;
        m_EnvironmentMap = envMap;
        m_ObjectDataOffset = 0;
        m_MaterialDataOffset = 0;
        m_TransparentMeshes.clear();

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

        // Opaque pass
        m_FinalFramebuffer->StartNextSubpass();
        RenderOpaque();

        // Transparent pass
        m_FinalFramebuffer->StartNextSubpass();
        RenderTransparent();

        // Composite pass
        m_FinalFramebuffer->StartNextSubpass();
        Composite();

        // Submit the framebuffer
        Renderer::Api().RenderFramebuffers(context, { m_FinalFramebuffer.get() });
    }

    void SceneRenderer::RenderEnvironmentMap()
    {
        m_FinalFramebuffer->BindPipeline("skybox");
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());
        m_FinalFramebuffer->BindShaderTextureResource(1, m_EnvironmentMap->GetPrefilterCubemap());

        auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("DefaultCube.gltf", true);
        auto& meshData = meshAsset->GetSubmesh(0);

        m_FinalFramebuffer->FlushBindings();

        Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
        Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
        Renderer::Api().DrawIndexed(
            meshData.GetIndexBuffer()->GetAllocatedCount(),
            meshData.GetVertexBuffer()->GetAllocatedCount(),
            0, 0, 1
        );
    }

    void SceneRenderer::RenderGrid()
    {
        // Bind grid pipeline
        m_FinalFramebuffer->BindPipeline("grid");

        // Bind frame data
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());

        m_FinalFramebuffer->FlushBindings();

        // Draw
        Renderer::Api().BindVertexBuffer(*m_GridVertices);
        Renderer::Api().BindIndexBuffer(*m_GridIndices);
        Renderer::Api().DrawIndexed(
            m_GridIndices->GetAllocatedCount(),
            m_GridVertices->GetAllocatedCount(),
            0, 0, 1
        );
    }
    
    void SceneRenderer::RenderOpaque()
    {
        // Bind opaque PBR pipeline
        m_FinalFramebuffer->BindPipeline("pbr");

        // Bind frame data
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());

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

        auto group = m_Scene->GetRegistry().group<TransformComponent, MeshComponent>();
        for (auto entity : group)
        {
            auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(mesh.Mesh);
            if (!meshAsset || !meshAsset->IsValid()) continue;

            m_FinalFramebuffer->BindShaderBufferResource(1, m_ObjectDataOffset, m_ObjectDataBuffer.get());

            // store transform at offset in buffer
            ObjectData objectData = { m_Scene->GetEntityCachedTransform({ m_Scene, entity }), static_cast<int>(entity) };
            m_ObjectDataBuffer->SetData(&objectData, 1, m_ObjectDataOffset);

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);

                Material* finalMaterial = &meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()]; // default material
                if (mesh.Materials.size() > meshData.GetMaterialIndex())
                {
                    auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(mesh.Materials[meshData.GetMaterialIndex()]);
                    if (materialAsset && materialAsset->IsValid())
                       finalMaterial = &materialAsset->GetMaterial();
                }

                if (finalMaterial)
                {
                    // Transparent materials get cached for the next pass
                    if (finalMaterial->IsTransparent())
                    {
                        m_TransparentMeshes.emplace_back(finalMaterial, mesh.Mesh, i, objectData);
                        continue;
                    }

                    auto& materialData = finalMaterial->GetMaterialData();

                    auto albedoAsset = AssetManager::RetrieveAsset<TextureAsset>(finalMaterial->GetAlbedoTexture());
                    materialData.SetHasAlbedo(albedoAsset && albedoAsset->IsValid());
                    if (materialData.HasAlbedo())
                        m_FinalFramebuffer->BindShaderTextureResource(3, albedoAsset->GetTexture());

                    auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(finalMaterial->GetMetallicRoughnessTexture());
                    materialData.SetHasMetallicRoughness(metallicRoughnessAsset && metallicRoughnessAsset->IsValid());
                    if (materialData.HasMetallicRoughness())
                        m_FinalFramebuffer->BindShaderTextureResource(4, metallicRoughnessAsset->GetTexture());

                    auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(finalMaterial->GetNormalTexture());
                    materialData.SetHasNormal(normalAsset && normalAsset->IsValid());
                    if (materialData.HasNormal())
                        m_FinalFramebuffer->BindShaderTextureResource(5, normalAsset->GetTexture());

                    auto emissiveAsset = AssetManager::RetrieveAsset<TextureAsset>(finalMaterial->GetEmissiveTexture());
                    materialData.SetHasEmissive(emissiveAsset && emissiveAsset->IsValid());
                    if (materialData.HasEmissive())
                        m_FinalFramebuffer->BindShaderTextureResource(6, emissiveAsset->GetTexture());

                    auto occlusionAsset = AssetManager::RetrieveAsset<TextureAsset>(finalMaterial->GetOcclusionTexture());
                    materialData.SetHasOcclusion(occlusionAsset && occlusionAsset->IsValid());
                    if (materialData.HasOcclusion())
                        m_FinalFramebuffer->BindShaderTextureResource(7, occlusionAsset->GetTexture());

                    m_MaterialDataBuffer->SetData(&materialData, 1, m_MaterialDataOffset);
                }
                else // default material
                    m_MaterialDataBuffer->SetData(&AssetManager::RetrieveAsset<MaterialAsset>("DefaultMaterial.hemat", true)->GetMaterial().GetMaterialData(), 1, m_MaterialDataOffset);

                m_FinalFramebuffer->BindShaderBufferResource(2, m_MaterialDataOffset, m_MaterialDataBuffer.get());

                m_FinalFramebuffer->FlushBindings();

                // Draw
                Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
                Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
                Renderer::Api().DrawIndexed(
                    meshData.GetIndexBuffer()->GetAllocatedCount(),
                    meshData.GetVertexBuffer()->GetAllocatedCount(),
                    0, 0, 1
                );

                m_MaterialDataOffset++;
            }

            m_ObjectDataOffset++;
        }
    }
    
    void SceneRenderer::RenderTransparent()
    {
        // Bind transparent PBR pipeline
        m_FinalFramebuffer->BindPipeline("pbrTpColor");
        
        // Bind frame data
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());

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

        for (auto& mesh : m_TransparentMeshes)
        {
            // We can safely load the assets here because the object must have a mesh & material to make it this far
            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(mesh.Mesh);
            auto& meshData = meshAsset->GetSubmesh(mesh.SubmeshIndex);
            auto& materialData = mesh.Material->GetMaterialData();

            auto albedoAsset = AssetManager::RetrieveAsset<TextureAsset>(mesh.Material->GetAlbedoTexture());
            materialData.SetHasAlbedo(albedoAsset && albedoAsset->IsValid());
            if (materialData.HasAlbedo())
                m_FinalFramebuffer->BindShaderTextureResource(3, albedoAsset->GetTexture());

            auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(mesh.Material->GetMetallicRoughnessTexture());
            materialData.SetHasMetallicRoughness(metallicRoughnessAsset && metallicRoughnessAsset->IsValid());
            if (materialData.HasMetallicRoughness())
                m_FinalFramebuffer->BindShaderTextureResource(4, metallicRoughnessAsset->GetTexture());

            auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(mesh.Material->GetNormalTexture());
            materialData.SetHasNormal(normalAsset && normalAsset->IsValid());
            if (materialData.HasNormal())
                m_FinalFramebuffer->BindShaderTextureResource(5, normalAsset->GetTexture());

            auto emissiveAsset = AssetManager::RetrieveAsset<TextureAsset>(mesh.Material->GetEmissiveTexture());
            materialData.SetHasEmissive(emissiveAsset && emissiveAsset->IsValid());
            if (materialData.HasEmissive())
                m_FinalFramebuffer->BindShaderTextureResource(6, emissiveAsset->GetTexture());

            auto occlusionAsset = AssetManager::RetrieveAsset<TextureAsset>(mesh.Material->GetOcclusionTexture());
            materialData.SetHasOcclusion(occlusionAsset && occlusionAsset->IsValid());
            if (materialData.HasOcclusion())
                m_FinalFramebuffer->BindShaderTextureResource(7, occlusionAsset->GetTexture());

            m_FinalFramebuffer->BindShaderBufferResource(1, m_ObjectDataOffset, m_ObjectDataBuffer.get());
            m_ObjectDataBuffer->SetData(&mesh.ObjectData, 1, m_ObjectDataOffset);

            m_FinalFramebuffer->BindShaderBufferResource(2, m_MaterialDataOffset, m_MaterialDataBuffer.get());
            m_MaterialDataBuffer->SetData(&materialData, 1, m_MaterialDataOffset);

            m_FinalFramebuffer->FlushBindings();

            // Draw
            Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
            Renderer::Api().DrawIndexed(
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                meshData.GetVertexBuffer()->GetAllocatedCount(),
                0, 0, 1
            );

            m_ObjectDataOffset++;
            m_MaterialDataOffset++;
        }
    }
    
    void SceneRenderer::Composite()
    {
        // Bind alpha compositing pipeline
        m_FinalFramebuffer->BindPipeline("tpComposite");

        // Bind frame data
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());

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