#include "htpch.h"
#include "SceneRenderer.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Camera.h"
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
        // register resources
        AssetManager::RegisterAsset(Asset::Type::Texture, "DefaultTexture.png", true, true);
        AssetManager::RegisterAsset(Asset::Type::Material, "DefaultMaterial.hemat", true, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "PBR.vert", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "PBR.frag", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "FullscreenTriangle.vert", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "PBRTransparentColor.frag", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "TransparentComposite.frag", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "Cubemap.vert", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "EnvironmentMap.frag", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "IrradianceMap.frag", false, true);
        AssetManager::RegisterAsset(Asset::Type::Mesh, "DefaultCube.gltf", false, true);

        // graphics pipeline
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
            0
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
            1
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
            2
        };

        // per frame data buffer layout
        BufferLayout frameDataLayout = {
            { BufferDataType::Mat4 },
            { BufferDataType::Mat4 },
            { BufferDataType::Float4 },
            { BufferDataType::Float2 },
            { BufferDataType::Bool },
            { BufferDataType::Bool },
            { BufferDataType::Float4 }, // padding
            { BufferDataType::Float4 }, // padding
        };

        // per object data buffer layout
        BufferLayout objectDataLayout = {
            { BufferDataType::Mat4 }, // transform
            { BufferDataType::Int }, // entityId
            { BufferDataType::Float3 } // padding
        };

        BufferLayout materialDataLayout = {
            { BufferDataType::Float4 },
            { BufferDataType::Float4 },
            { BufferDataType::Float4 },
            { BufferDataType::Float4 }
        };

        m_FrameDataBuffer = Buffer::Create(Buffer::Type::Uniform, BufferUsageType::Dynamic, frameDataLayout, 50, nullptr);
        m_ObjectDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, objectDataLayout, 2000, nullptr);
        m_MaterialDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, materialDataLayout, 2000, nullptr);

        // framebuffer
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
                { {}, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 0 }, { SubpassAttachmentType::Color, 1 } } }, // opaque
                { {}, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 1 }, { SubpassAttachmentType::Color, 2 }, { SubpassAttachmentType::Color, 3 } } }, // transparent color
                { { { SubpassAttachmentType::Color, 2 }, { SubpassAttachmentType::Color, 3 } }, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 0 } } } // composite
            }
        };
        fbCreateInfo.Width = 0;
        fbCreateInfo.Height = 0;
        fbCreateInfo.SampleCount = MsaaSampleCount::None;
        m_FinalFramebuffer = Framebuffer::Create(fbCreateInfo);
        m_FinalFramebuffer->RegisterGraphicsPipeline("pbr", pbrPipeline);
        m_FinalFramebuffer->RegisterGraphicsPipeline("pbrTpColor", transparencyColorPipeline);
        m_FinalFramebuffer->RegisterGraphicsPipeline("tpComposite", transparencyCompositePipeline);

        // ------------------------------------------------------------------
        // Environment map initalization
        // ------------------------------------------------------------------

        GraphicsPipelineCreateInfo envPipeline = {
            AssetManager::GetAssetUUID("Cubemap.vert", true),
            AssetManager::GetAssetUUID("EnvironmentMap.frag", true),
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

        m_EnvironmentMap = Texture::Create(512, 512, 4, nullptr, 6, true);

        // cubemap framebuffer
        FramebufferCreateInfo cubemapFbCreateInfo = {
            {
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_EnvironmentMap, 0 }
            },
            {},
            {
                { {}, { { SubpassAttachmentType::Color, 0 } } },
            }
        };
        cubemapFbCreateInfo.Width = m_EnvironmentMap->GetWidth();
        cubemapFbCreateInfo.Height = m_EnvironmentMap->GetHeight();
        cubemapFbCreateInfo.SampleCount = MsaaSampleCount::None;
        for (u32 i = 0; i < 6; i++)
        {
            cubemapFbCreateInfo.ColorAttachments[0].LayerIndex = i;
            m_CubemapFramebuffers.emplace_back(Framebuffer::Create(cubemapFbCreateInfo));
            m_CubemapFramebuffers[i]->RegisterGraphicsPipeline("envmap", envPipeline);
        }
    }

    SceneRenderer::~SceneRenderer()
    {
        
    }

    void SceneRenderer::CalculateEnvironmentMaps(GraphicsContext& context)
    {
        auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("DefaultCube.gltf", true);
        auto& meshData = meshAsset->GetSubmesh(0);

        Camera cubemapCam(90.f, 0.1f, 50.f, 1.f);

        glm::vec2 rotations[] = { { 0.f, 0.f }, { 0.f, 90.f }, { 0.f, 180.f }, { 0.f, 270.f }, { 90.f, 0.f }, { -90.f, 0.f } };
        for (u32 i = 0; i < 6; i++)
        {
            m_CubemapFramebuffers[i]->Bind();
            m_CubemapFramebuffers[i]->BindPipeline("envmap");  

            cubemapCam.UpdateViewMatrix(rotations[i].x, rotations[i].y, { 0.f, 0.f, 0.f });

            FrameData frameData = { cubemapCam.GetProjectionMatrix(), cubemapCam.GetViewMatrix(), glm::vec4(0.f, 0.f, 0.f, 1.f), m_CubemapFramebuffers[i]->GetSize(), Renderer::IsUsingReverseDepth() };
            m_FrameDataBuffer->SetData(&frameData, 1, i);
            m_CubemapFramebuffers[i]->BindShaderBufferResource(0, i, m_FrameDataBuffer.get());

            m_CubemapFramebuffers[i]->BindShaderTextureResource(1, AssetManager::RetrieveAsset<TextureAsset>("assets/envmaps/GrandCanyon.hdr")->GetTexture());

            Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
            Renderer::Api().DrawIndexed(
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                meshData.GetVertexBuffer()->GetAllocatedCount(),
                0, 0, 1
            );

            Renderer::Api().RenderFramebuffers(context, { m_CubemapFramebuffers[i].get() });
        }
    }

    void SceneRenderer::RenderScene(GraphicsContext& context, Scene* scene, glm::mat4 view, glm::mat4 projection, glm::vec3 position)
    {
        HE_PROFILE_FUNCTION();

        HE_ENGINE_ASSERT(scene, "Scene cannot be nullptr");

        if (App::Get().GetFrameCount() == 0)
            CalculateEnvironmentMaps(context);

        m_FinalFramebuffer->Bind();
        m_FinalFramebuffer->BindPipeline("pbr");

        FrameData frameData = { projection, view, glm::vec4(position, 1.f), m_FinalFramebuffer->GetSize(), Renderer::IsUsingReverseDepth() };
        m_FrameDataBuffer->SetData(&frameData, 1, 0);

        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());

        // default texture binds
        m_FinalFramebuffer->BindShaderTextureResource(3, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(4, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(5, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());

        std::vector<CachedRender> transparentMeshes;
        auto group = scene->GetRegistry().group<TransformComponent, MeshComponent>();
        u32 objectIndex = 0;
        u32 materialIndex = 0;
        for (auto entity : group)
        {
            auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(mesh.Mesh);
            if (!meshAsset || !meshAsset->IsValid()) continue;

            m_FinalFramebuffer->BindShaderBufferResource(1, objectIndex, m_ObjectDataBuffer.get());

            // store transform at offset in buffer
            ObjectData objectData = { scene->GetEntityCachedTransform({ scene, entity }), static_cast<int>(entity) };
            m_ObjectDataBuffer->SetData(&objectData, 1, objectIndex);

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);

                UUID finalMaterialId = meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()];
                if (mesh.Materials.size() > meshData.GetMaterialIndex())
                    finalMaterialId = mesh.Materials[meshData.GetMaterialIndex()];

                auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(finalMaterialId);
                if (materialAsset && materialAsset->IsValid())
                {
                    if (materialAsset->GetMaterial().IsTransparent())
                    {
                        transparentMeshes.emplace_back(finalMaterialId, mesh.Mesh, i, objectData);
                        continue;
                    }

                    auto& materialData = materialAsset->GetMaterial().GetMaterialData();

                    auto albedoAsset = AssetManager::RetrieveAsset<TextureAsset>(materialAsset->GetMaterial().GetAlbedoTexture());
                    materialData.SetHasAlbedo(albedoAsset && albedoAsset->IsValid());
                    if (materialData.HasAlbedo())
                        m_FinalFramebuffer->BindShaderTextureResource(3, albedoAsset->GetTexture());

                    auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(materialAsset->GetMaterial().GetMetallicRoughnessTexture());
                    materialData.SetHasMetallicRoughness(metallicRoughnessAsset && metallicRoughnessAsset->IsValid());
                    if (materialData.HasMetallicRoughness())
                        m_FinalFramebuffer->BindShaderTextureResource(4, metallicRoughnessAsset->GetTexture());

                    auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(materialAsset->GetMaterial().GetNormalTexture());
                    materialData.SetHasNormal(normalAsset && normalAsset->IsValid());
                    if (materialData.HasNormal())
                        m_FinalFramebuffer->BindShaderTextureResource(5, normalAsset->GetTexture());

                    m_MaterialDataBuffer->SetData(&materialData, 1, materialIndex);
                }
                else // default material
                    m_MaterialDataBuffer->SetData(&AssetManager::RetrieveAsset<MaterialAsset>("DefaultMaterial.hemat", true)->GetMaterial().GetMaterialData(), 1, materialIndex);

                m_FinalFramebuffer->BindShaderBufferResource(2, materialIndex, m_MaterialDataBuffer.get());

                Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
                Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());

                // draw
                Renderer::Api().DrawIndexed(
                    meshData.GetIndexBuffer()->GetAllocatedCount(),
                    meshData.GetVertexBuffer()->GetAllocatedCount(),
                    0, 0, 1
                );

                materialIndex++;
            }

            objectIndex++;
        }

        m_FinalFramebuffer->StartNextSubpass();
        m_FinalFramebuffer->BindPipeline("pbrTpColor");
        
        // default texture binds
        m_FinalFramebuffer->BindShaderTextureResource(3, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());

        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());

        for (auto& mesh : transparentMeshes)
        {
            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(mesh.Mesh);
            auto& meshData = meshAsset->GetSubmesh(mesh.SubmeshIndex);
            auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(mesh.Material);
            auto& materialData = materialAsset->GetMaterial().GetMaterialData();

            auto albedoAsset = AssetManager::RetrieveAsset<TextureAsset>(materialAsset->GetMaterial().GetAlbedoTexture());
            materialData.SetHasAlbedo(albedoAsset && albedoAsset->IsValid());
            if (materialData.HasAlbedo())
                m_FinalFramebuffer->BindShaderTextureResource(3, albedoAsset->GetTexture());

            auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(materialAsset->GetMaterial().GetMetallicRoughnessTexture());
            materialData.SetHasMetallicRoughness(metallicRoughnessAsset && metallicRoughnessAsset->IsValid());
            if (materialData.HasMetallicRoughness())
                m_FinalFramebuffer->BindShaderTextureResource(4, metallicRoughnessAsset->GetTexture());

            auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(materialAsset->GetMaterial().GetNormalTexture());
            materialData.SetHasNormal(normalAsset && normalAsset->IsValid());
            if (materialData.HasNormal())
                m_FinalFramebuffer->BindShaderTextureResource(5, normalAsset->GetTexture());

            m_FinalFramebuffer->BindShaderBufferResource(1, objectIndex, m_ObjectDataBuffer.get());
            m_ObjectDataBuffer->SetData(&mesh.ObjectData, 1, objectIndex);

            m_FinalFramebuffer->BindShaderBufferResource(2, materialIndex, m_MaterialDataBuffer.get());
            m_MaterialDataBuffer->SetData(&materialData, 1, materialIndex);

            Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());

            // draw
            Renderer::Api().DrawIndexed(
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                meshData.GetVertexBuffer()->GetAllocatedCount(),
                0, 0, 1
            );

            objectIndex++;
            materialIndex++;
        }
        
        m_FinalFramebuffer->StartNextSubpass();
        m_FinalFramebuffer->BindPipeline("tpComposite");

        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());
        m_FinalFramebuffer->BindSubpassInputAttachment(1, { SubpassAttachmentType::Color, 2 });
        m_FinalFramebuffer->BindSubpassInputAttachment(2, { SubpassAttachmentType::Color, 3 });

        Renderer::Api().Draw(3, 0, 1);

        Renderer::Api().RenderFramebuffers(context, { m_FinalFramebuffer.get() });
    }
}