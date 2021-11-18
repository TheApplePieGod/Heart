#include "htpch.h"
#include "SceneRenderer.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Asset/MeshAsset.h"
#include "glm/gtc/matrix_transform.hpp"

namespace Heart
{
    SceneRenderer::SceneRenderer()
    {
        // register default shaders
        AssetManager::RegisterAsset(Asset::Type::Shader, "assets/shaders/main.vert");
        AssetManager::RegisterAsset(Asset::Type::Shader, "assets/shaders/main.frag");
        AssetManager::RegisterAsset(Asset::Type::Shader, "assets/shaders/color.frag");
        AssetManager::RegisterAsset(Asset::Type::Shader, "assets/shaders/composite.frag");
        AssetManager::RegisterAsset(Asset::Type::Shader, "assets/shaders/fulltriangle.vert");

        // register testing assets
        AssetManager::RegisterAsset(Asset::Type::Texture, "assets/textures/fish.png");
        AssetManager::RegisterAsset(Asset::Type::Texture, "assets/textures/test.png");
        AssetManager::RegisterAsset(Asset::Type::Mesh, "assets/meshes/cube.gltf");
        AssetManager::RegisterAsset(Asset::Type::Mesh, "assets/meshes/Sponza/glTF/Sponza.gltf");
        AssetManager::RegisterAsset(Asset::Type::Mesh, "assets/meshes/Buggy/glTF/Buggy.gltf");
        AssetManager::RegisterAsset(Asset::Type::Mesh, "assets/meshes/Duck/glTF/Duck.gltf");
        AssetManager::RegisterAsset(Asset::Type::Mesh, "assets/meshes/IridescentDishWithOlives/glTF/IridescentDishWithOlives.gltf");

        // graphics pipeline
        GraphicsPipelineCreateInfo mainPipeline = {
            "assets/shaders/main.vert",
            "assets/shaders/main.frag",
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
            "assets/shaders/main.vert",
            "assets/shaders/color.frag",
            true,
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { false }, { true, BlendFactor::One, BlendFactor::One, BlendFactor::One, BlendFactor::One }, { true, BlendFactor::Zero, BlendFactor::OneMinusSrcColor, BlendFactor::Zero, BlendFactor::OneMinusSrcAlpha } },
            true,
            false,
            CullMode::None,
            WindingOrder::Clockwise,
            1
        };
        GraphicsPipelineCreateInfo transparencyCompositePipeline = {
            "assets/shaders/fulltriangle.vert",
            "assets/shaders/composite.frag",
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
            { BufferDataType::Float2 },
            { BufferDataType::Float2 }
        };

        // per object data buffer layout
        BufferLayout objectDataLayout = {
            { BufferDataType::Mat4, BufferDataType::Int, BufferDataType::Float3 },
        };

        // per frame data buffer
        m_FrameDataBuffer = Buffer::Create(Buffer::Type::Uniform, BufferUsageType::Dynamic, frameDataLayout, 1, nullptr);

        // object data buffer
        m_ObjectDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, objectDataLayout, 2000, nullptr);

        // framebuffer
        FramebufferCreateInfo fbCreateInfo = {
            {
                { { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::RGBA8, false },
                { { -1.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::R32F, true },
                { { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::RGBA32F, false },
                { { 1.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::R32F, false }
            },
            {
                { false }
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
        m_FinalFramebuffer->RegisterGraphicsPipeline("main", mainPipeline);
        m_FinalFramebuffer->RegisterGraphicsPipeline("tpColor", transparencyColorPipeline);
        m_FinalFramebuffer->RegisterGraphicsPipeline("tpComposite", transparencyCompositePipeline);
    }

    SceneRenderer::~SceneRenderer()
    {
        
    }

    void SceneRenderer::RenderScene(GraphicsContext& context, Scene* scene, glm::mat4 view, glm::mat4 viewProjection)
    {
        HE_PROFILE_FUNCTION();

        HE_ENGINE_ASSERT(scene, "Scene cannot be nullptr");

        m_FinalFramebuffer->Bind();
        m_FinalFramebuffer->BindPipeline("main");

        FrameData frameData = { viewProjection, view, m_FinalFramebuffer->GetSize(), { 0.f, 0.f } };
        m_FrameDataBuffer->SetData(&frameData, 1, 0);

        // all shader resources must be bound before drawing
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());

        // bind default texture
        m_FinalFramebuffer->BindShaderTextureResource(2, AssetManager::RetrieveAsset<TextureAsset>("assets/textures/test.png")->GetTexture());

        std::vector<CachedRender> transparentMeshes;
        auto group = scene->GetRegistry().group<TransformComponent, MeshComponent>();
        u32 index = 0;
        for (auto entity : group)
        {
            auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(mesh.MeshPath);
            if (!meshAsset) continue;

            m_FinalFramebuffer->BindShaderBufferResource(1, index, m_ObjectDataBuffer.get());

            // store transform at offset in buffer
            ObjectData objectData = { scene->GetEntityCachedTransform({ scene, entity }), static_cast<int>(entity), { 0.f, 0.f, 0.f } };
            m_ObjectDataBuffer->SetData(&objectData, 1, index);

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);

                std::string finalTexturePath = meshAsset->GetDefaultTexturePaths()[meshData.GetMaterialIndex()];
                if (mesh.TexturePaths.size() > meshData.GetMaterialIndex())
                    finalTexturePath = mesh.TexturePaths[meshData.GetMaterialIndex()];

                auto textureAsset = AssetManager::RetrieveAsset<TextureAsset>(finalTexturePath);
                if (textureAsset)
                {
                    if (textureAsset->GetTexture()->HasTransparency())
                    {
                        transparentMeshes.emplace_back(finalTexturePath, mesh.MeshPath, i, objectData);
                        continue;
                    }
                    m_FinalFramebuffer->BindShaderTextureResource(2, textureAsset->GetTexture());
                }

                Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
                Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());

                // draw
                Renderer::Api().DrawIndexed(
                    meshData.GetIndexBuffer()->GetAllocatedCount(),
                    meshData.GetVertexBuffer()->GetAllocatedCount(),
                    0, 0, 1
                );
            }

            index++;
        }

        m_FinalFramebuffer->StartNextSubpass();
        m_FinalFramebuffer->BindPipeline("tpColor");
        
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());

        for (auto& mesh : transparentMeshes)
        {
            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(mesh.MeshPath);
            auto& meshData = meshAsset->GetSubmesh(mesh.SubmeshIndex);
            auto textureAsset = AssetManager::RetrieveAsset<TextureAsset>(mesh.TexturePath);

            m_FinalFramebuffer->BindShaderTextureResource(2, textureAsset->GetTexture());

            Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());

            m_FinalFramebuffer->BindShaderBufferResource(1, index, m_ObjectDataBuffer.get());

            // update transform of each cube within the buffer
            m_ObjectDataBuffer->SetData(&mesh.ObjectData, 1, index);

            // draw
            Renderer::Api().DrawIndexed(
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                meshData.GetVertexBuffer()->GetAllocatedCount(),
                0, 0, 1
            );

            index++;
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