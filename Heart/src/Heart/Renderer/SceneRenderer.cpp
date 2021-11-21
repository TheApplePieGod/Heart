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
        AssetManager::RegisterAsset(Asset::Type::Shader, "CalcEnvironmentMap.frag", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "EnvironmentMap.vert", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "EnvironmentMap.frag", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "CalcIrradianceMap.frag", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "CalcPrefilterMap.frag", false, true);
        AssetManager::RegisterAsset(Asset::Type::Shader, "CalcBRDF.frag", false, true);
        AssetManager::RegisterAsset(Asset::Type::Mesh, "DefaultCube.gltf", false, true);

        // graphics pipeline
        GraphicsPipelineCreateInfo envMapPipeline = {
            AssetManager::GetAssetUUID("EnvironmentMap.vert", true),
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
            1
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
            2
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
            3
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
                { {}, { { SubpassAttachmentType::Color, 0 } } }, // environment map
                { {}, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 0 }, { SubpassAttachmentType::Color, 1 } } }, // opaque
                { {}, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 1 }, { SubpassAttachmentType::Color, 2 }, { SubpassAttachmentType::Color, 3 } } }, // transparent color
                { { { SubpassAttachmentType::Color, 2 }, { SubpassAttachmentType::Color, 3 } }, { { SubpassAttachmentType::Depth, 0 }, { SubpassAttachmentType::Color, 0 } } } // composite
            },
            0, 0,
            MsaaSampleCount::None
        };
        m_FinalFramebuffer = Framebuffer::Create(fbCreateInfo);
        m_FinalFramebuffer->RegisterGraphicsPipeline("envMap", envMapPipeline);
        m_FinalFramebuffer->RegisterGraphicsPipeline("pbr", pbrPipeline);
        m_FinalFramebuffer->RegisterGraphicsPipeline("pbrTpColor", transparencyColorPipeline);
        m_FinalFramebuffer->RegisterGraphicsPipeline("tpComposite", transparencyCompositePipeline);

        // ------------------------------------------------------------------
        // Environment map initalization
        // ------------------------------------------------------------------

        GraphicsPipelineCreateInfo envPipeline = {
            AssetManager::GetAssetUUID("EnvironmentMap.vert", true),
            AssetManager::GetAssetUUID("CalcEnvironmentMap.frag", true),
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

        m_EnvironmentMap = Texture::Create({ 512, 512, 4, true, 6, 1 });
        m_IrradianceMap = Texture::Create({ 256, 256, 4, true, 6, 1 });
        m_PrefilterMap = Texture::Create({ 256, 256, 4, true, 6, 5 });
        m_BRDFTexture = Texture::Create({ 512, 512, 4, true, 1, 1 });

        // cubemap framebuffer
        FramebufferCreateInfo cubemapFbCreateInfo = {
            {
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_EnvironmentMap, 0 },
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_EnvironmentMap, 1 },
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_EnvironmentMap, 2 },
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_EnvironmentMap, 3 },
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_EnvironmentMap, 4 },
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_EnvironmentMap, 5 }
            },
            {},
            {
                { {}, { { SubpassAttachmentType::Color, 0 } } },
                { {}, { { SubpassAttachmentType::Color, 1 } } },
                { {}, { { SubpassAttachmentType::Color, 2 } } },
                { {}, { { SubpassAttachmentType::Color, 3 } } },
                { {}, { { SubpassAttachmentType::Color, 4 } } },
                { {}, { { SubpassAttachmentType::Color, 5 } } }
            },
            m_EnvironmentMap->GetWidth(), m_EnvironmentMap->GetHeight(),
            MsaaSampleCount::None
        };
        m_CubemapFramebuffer = Framebuffer::Create(cubemapFbCreateInfo);
        for (u32 i = 0; i < 6; i++)
        {
            envPipeline.SubpassIndex = i;
            m_CubemapFramebuffer->RegisterGraphicsPipeline(std::to_string(i), envPipeline);
        }

        GraphicsPipelineCreateInfo irradiancePipeline = {
            AssetManager::GetAssetUUID("EnvironmentMap.vert", true),
            AssetManager::GetAssetUUID("CalcIrradianceMap.frag", true),
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

        // irradiance framebuffer
        FramebufferCreateInfo irradianceFbCreateInfo = {
            {
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_IrradianceMap, 0 },
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_IrradianceMap, 1 },
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_IrradianceMap, 2 },
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_IrradianceMap, 3 },
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_IrradianceMap, 4 },
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_IrradianceMap, 5 }
            },
            {},
            {
                { {}, { { SubpassAttachmentType::Color, 0 } } },
                { {}, { { SubpassAttachmentType::Color, 1 } } },
                { {}, { { SubpassAttachmentType::Color, 2 } } },
                { {}, { { SubpassAttachmentType::Color, 3 } } },
                { {}, { { SubpassAttachmentType::Color, 4 } } },
                { {}, { { SubpassAttachmentType::Color, 5 } } }
            },
            m_IrradianceMap->GetWidth(), m_IrradianceMap->GetHeight(),
            MsaaSampleCount::None
        };
        m_IrradianceMapFramebuffer = Framebuffer::Create(irradianceFbCreateInfo);
        for (u32 i = 0; i < 6; i++)
        {
            irradiancePipeline.SubpassIndex = i;
            m_IrradianceMapFramebuffer->RegisterGraphicsPipeline(std::to_string(i), irradiancePipeline);
        }

        GraphicsPipelineCreateInfo prefilterPipeline = {
            AssetManager::GetAssetUUID("EnvironmentMap.vert", true),
            AssetManager::GetAssetUUID("CalcPrefilterMap.frag", true),
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

        // prefilter framebuffer
        FramebufferCreateInfo prefilterFbCreateInfo = {
            {}, {},
            {
                { {}, { { SubpassAttachmentType::Color, 0 } } },
                { {}, { { SubpassAttachmentType::Color, 1 } } },
                { {}, { { SubpassAttachmentType::Color, 2 } } },
                { {}, { { SubpassAttachmentType::Color, 3 } } },
                { {}, { { SubpassAttachmentType::Color, 4 } } },
                { {}, { { SubpassAttachmentType::Color, 5 } } }
            },
            0, 0,
            MsaaSampleCount::None
        };
        for (u32 i = 0; i < 5; i++) // each mip level
        {
            prefilterFbCreateInfo.ColorAttachments.clear();
            for (u32 j = 0; j < 6; j++) // each face
                prefilterFbCreateInfo.ColorAttachments.push_back({ false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_PrefilterMap, j, i });
            prefilterFbCreateInfo.Width = static_cast<u32>(m_PrefilterMap->GetWidth() * pow(0.5f, i));
            prefilterFbCreateInfo.Height = static_cast<u32>(m_PrefilterMap->GetHeight() * pow(0.5f, i));

            m_PrefilterFramebuffers.emplace_back(Framebuffer::Create(prefilterFbCreateInfo));
            for (u32 j = 0; j < 6; j++) // each face
            {
                prefilterPipeline.SubpassIndex = j;
                m_PrefilterFramebuffers.back()->RegisterGraphicsPipeline(std::to_string(j), prefilterPipeline);
            }
        }

        GraphicsPipelineCreateInfo brdfPipeline = {
            AssetManager::GetAssetUUID("FullscreenTriangle.vert", true),
            AssetManager::GetAssetUUID("CalcBRDF.frag", true),
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

        // brdf framebuffer
        FramebufferCreateInfo brdfFbCreateInfo = {
            {
                { false, { 0.f, 0.f, 0.f, 0.f }, ColorFormat::None, m_BRDFTexture }
            },
            {},
            {
                { {}, { { SubpassAttachmentType::Color, 0 } } }
            },
            m_BRDFTexture->GetWidth(), m_BRDFTexture->GetHeight(),
            MsaaSampleCount::None
        };
        m_BRDFFramebuffer = Framebuffer::Create(brdfFbCreateInfo);
        m_BRDFFramebuffer->RegisterGraphicsPipeline("0", brdfPipeline);
    }

    SceneRenderer::~SceneRenderer()
    {
        
    }

    void SceneRenderer::CalculateEnvironmentMaps(GraphicsContext& context)
    {
        auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("DefaultCube.gltf", true);
        auto& meshData = meshAsset->GetSubmesh(0);

        // +X -X +Y -Y +Z -Z
        glm::vec2 rotations[] = { { 90.f, 0.f }, { -90.f, 0.f }, { 0.f, 90.f }, { 0.f, -90.f }, { 0.f, 0.f }, { 180.f, 0.f } };
        Camera cubemapCam(90.f, 0.1f, 50.f, 1.f);
        u32 frameDataIndex = 0;

        // ------------------------------------------------------------------
        // Render equirectangular map to cubemap
        // ------------------------------------------------------------------

        m_CubemapFramebuffer->Bind();
        for (u32 i = 0; i < 6; i++)
        {
            if (i != 0)
                m_CubemapFramebuffer->StartNextSubpass();

            m_CubemapFramebuffer->BindPipeline(std::to_string(i));  

            cubemapCam.UpdateViewMatrix(rotations[i].x, rotations[i].y, { 0.f, 0.f, 0.f });

            FrameData frameData = { cubemapCam.GetProjectionMatrix(), cubemapCam.GetViewMatrix(), glm::vec4(0.f, 0.f, 0.f, 1.f), m_CubemapFramebuffer->GetSize(), Renderer::IsUsingReverseDepth() };
            m_FrameDataBuffer->SetData(&frameData, 1, frameDataIndex);
            m_CubemapFramebuffer->BindShaderBufferResource(0, frameDataIndex, m_FrameDataBuffer.get());

            m_CubemapFramebuffer->BindShaderTextureResource(1, AssetManager::RetrieveAsset<TextureAsset>("assets/envmaps/GrandCanyon.hdr")->GetTexture());

            Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
            Renderer::Api().DrawIndexed(
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                meshData.GetVertexBuffer()->GetAllocatedCount(),
                0, 0, 1
            );

            frameDataIndex++;
        }

        Renderer::Api().RenderFramebuffers(context, { m_CubemapFramebuffer.get() });

        // ------------------------------------------------------------------
        // Precalculate environment irradiance
        // ------------------------------------------------------------------

        m_IrradianceMapFramebuffer->Bind();
        for (u32 i = 0; i < 6; i++)
        {
            if (i != 0)
                m_IrradianceMapFramebuffer->StartNextSubpass();

            m_IrradianceMapFramebuffer->BindPipeline(std::to_string(i));  

            cubemapCam.UpdateViewMatrix(rotations[i].x, rotations[i].y, { 0.f, 0.f, 0.f });

            FrameData frameData = { cubemapCam.GetProjectionMatrix(), cubemapCam.GetViewMatrix(), glm::vec4(0.f, 0.f, 0.f, 1.f), m_IrradianceMapFramebuffer->GetSize(), Renderer::IsUsingReverseDepth() };
            m_FrameDataBuffer->SetData(&frameData, 1, frameDataIndex);
            m_IrradianceMapFramebuffer->BindShaderBufferResource(0, frameDataIndex, m_FrameDataBuffer.get());

            m_IrradianceMapFramebuffer->BindShaderTextureResource(1, m_EnvironmentMap.get());

            Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
            Renderer::Api().DrawIndexed(
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                meshData.GetVertexBuffer()->GetAllocatedCount(),
                0, 0, 1
            );

            frameDataIndex++;
        }

        Renderer::Api().RenderFramebuffers(context, { m_IrradianceMapFramebuffer.get() });

        // ------------------------------------------------------------------
        // Prefilter the environment map based on roughness
        // ------------------------------------------------------------------

        for (size_t i = 0; i < m_PrefilterFramebuffers.size(); i++)
        {
            m_PrefilterFramebuffers[i]->Bind();

            float roughness = static_cast<float>(i) / 4;
            for (u32 j = 0; j < 6; j++) // each face
            {
                if (j != 0)
                    m_PrefilterFramebuffers[i]->StartNextSubpass();

                m_PrefilterFramebuffers[i]->BindPipeline(std::to_string(j));  

                cubemapCam.UpdateViewMatrix(rotations[j].x, rotations[j].y, { 0.f, 0.f, 0.f });

                FrameData frameData = { cubemapCam.GetProjectionMatrix(), cubemapCam.GetViewMatrix(), glm::vec4(roughness, 0.f, 0.f, 1.f), m_PrefilterFramebuffers[i]->GetSize(), Renderer::IsUsingReverseDepth() };
                m_FrameDataBuffer->SetData(&frameData, 1, frameDataIndex);
                m_PrefilterFramebuffers[i]->BindShaderBufferResource(0, frameDataIndex, m_FrameDataBuffer.get());

                m_PrefilterFramebuffers[i]->BindShaderTextureResource(1, m_EnvironmentMap.get());

                Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
                Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
                Renderer::Api().DrawIndexed(
                    meshData.GetIndexBuffer()->GetAllocatedCount(),
                    meshData.GetVertexBuffer()->GetAllocatedCount(),
                    0, 0, 1
                );

                frameDataIndex++;
            }

            Renderer::Api().RenderFramebuffers(context, { m_PrefilterFramebuffers[i].get() });
        }

        // ------------------------------------------------------------------
        // Precalculate the BRDF texture
        // ------------------------------------------------------------------

        m_BRDFFramebuffer->Bind();
        m_BRDFFramebuffer->BindPipeline("0");  

        FrameData frameData = { cubemapCam.GetProjectionMatrix(), cubemapCam.GetViewMatrix(), glm::vec4(0.f, 0.f, 0.f, 1.f), m_BRDFFramebuffer->GetSize(), Renderer::IsUsingReverseDepth() };
        m_FrameDataBuffer->SetData(&frameData, 1, frameDataIndex);
        m_BRDFFramebuffer->BindShaderBufferResource(0, frameDataIndex, m_FrameDataBuffer.get());

        Renderer::Api().Draw(3, 0, 1);
        Renderer::Api().RenderFramebuffers(context, { m_BRDFFramebuffer.get() });
        frameDataIndex++;
    }

    void SceneRenderer::RenderScene(GraphicsContext& context, Scene* scene, glm::mat4 view, glm::mat4 projection, glm::vec3 position)
    {
        HE_PROFILE_FUNCTION();

        HE_ENGINE_ASSERT(scene, "Scene cannot be nullptr");

        if (App::Get().GetFrameCount() == 0)
        {
            CalculateEnvironmentMaps(context);
            return;
        }

        FrameData frameData = { projection, view, glm::vec4(position, 1.f), m_FinalFramebuffer->GetSize(), Renderer::IsUsingReverseDepth() };
        m_FrameDataBuffer->SetData(&frameData, 1, 0);

        {
            m_FinalFramebuffer->Bind();
            m_FinalFramebuffer->BindPipeline("envMap");
            m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());
            m_FinalFramebuffer->BindShaderTextureResource(1, m_PrefilterMap.get());

            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("DefaultCube.gltf", true);
            auto& meshData = meshAsset->GetSubmesh(0);

            Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
            Renderer::Api().DrawIndexed(
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                meshData.GetVertexBuffer()->GetAllocatedCount(),
                0, 0, 1
            );
        }

        m_FinalFramebuffer->StartNextSubpass();

        m_FinalFramebuffer->BindPipeline("pbr");
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());

        // default texture binds
        m_FinalFramebuffer->BindShaderTextureResource(3, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(4, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(5, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(6, m_IrradianceMap.get());
        m_FinalFramebuffer->BindShaderTextureResource(7, m_PrefilterMap.get());
        m_FinalFramebuffer->BindShaderTextureResource(8, m_BRDFTexture.get());

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
        m_FinalFramebuffer->BindShaderTextureResource(4, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(5, AssetManager::RetrieveAsset<TextureAsset>("DefaultTexture.png", true)->GetTexture());
        m_FinalFramebuffer->BindShaderTextureResource(6, m_IrradianceMap.get());
        m_FinalFramebuffer->BindShaderTextureResource(7, m_PrefilterMap.get());
        m_FinalFramebuffer->BindShaderTextureResource(8, m_BRDFTexture.get());

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