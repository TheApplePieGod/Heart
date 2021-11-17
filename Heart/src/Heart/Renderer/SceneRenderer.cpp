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

        // register testing assets
        AssetManager::RegisterAsset(Asset::Type::Texture, "assets/textures/fish.png");
        AssetManager::RegisterAsset(Asset::Type::Texture, "assets/textures/test.png");
        AssetManager::RegisterAsset(Asset::Type::Mesh, "assets/meshes/cube.gltf");
        AssetManager::RegisterAsset(Asset::Type::Mesh, "assets/meshes/Sponza/glTF/Sponza.gltf");
        AssetManager::RegisterAsset(Asset::Type::Mesh, "assets/meshes/Buggy/glTF/Buggy.gltf");
        AssetManager::RegisterAsset(Asset::Type::Mesh, "assets/meshes/Duck/glTF/Duck.gltf");
        AssetManager::RegisterAsset(Asset::Type::Mesh, "assets/meshes/IridescentDishWithOlives/glTF/IridescentDishWithOlives.gltf");

        // graphics pipeline
        GraphicsPipelineCreateInfo gpCreateInfo = {
            "assets/shaders/main.vert",
            "assets/shaders/main.frag",
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { false }, { false }, { false } },
            true,
            CullMode::Backface,
            WindingOrder::Clockwise
        };

        // per frame data buffer layout
        BufferLayout frameDataLayout = {
            { BufferDataType::Mat4 }
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
        // todo: specificially create depth attachments & allow them to be referenced in subpass
        // input: color/depth
        // output
        // outputDepth
        FramebufferCreateInfo fbCreateInfo = {
            {
                { { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::RGBA8, false },
                { { -1.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::R32F, true },
                { { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::R32F, false },
                { { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::RGBA32F, false }
            },
            { { {}, { 0, 1, 2 } }, { { 2 }, { 0 } } }
        };
        fbCreateInfo.Width = 0;
        fbCreateInfo.Height = 0;
        fbCreateInfo.SampleCount = MsaaSampleCount::None;
        fbCreateInfo.HasDepth = true;
        m_FinalFramebuffer = Framebuffer::Create(fbCreateInfo);
        m_FinalFramebuffer->RegisterGraphicsPipeline("main", gpCreateInfo);
    }

    SceneRenderer::~SceneRenderer()
    {
        
    }

    void SceneRenderer::RenderScene(GraphicsContext& context, Scene* scene, glm::mat4 viewProjection)
    {
        HE_PROFILE_FUNCTION();

        HE_ENGINE_ASSERT(scene, "Scene cannot be nullptr");

        m_FinalFramebuffer->Bind();
        m_FinalFramebuffer->BindPipeline("main");

        m_FrameDataBuffer->SetData(&viewProjection, 1, 0);

        // all shader resources must be bound before drawing
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());

        // bind default texture
        m_FinalFramebuffer->BindShaderTextureResource(2, AssetManager::RetrieveAsset<TextureAsset>("assets/textures/test.png")->GetTexture());

        auto group = scene->GetRegistry().group<TransformComponent, MeshComponent>();
        u32 index = 0;
        for (auto entity : group)
        {
            auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(mesh.MeshPath);
            if (!meshAsset) continue;

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);
                Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
                Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());

                std::string finalTexturePath = meshAsset->GetDefaultTexturePaths()[meshData.GetMaterialIndex()];
                if (mesh.TexturePaths.size() > meshData.GetMaterialIndex())
                    finalTexturePath = mesh.TexturePaths[meshData.GetMaterialIndex()];

                auto textureAsset = AssetManager::RetrieveAsset<TextureAsset>(finalTexturePath);
                if (textureAsset)
                    m_FinalFramebuffer->BindShaderTextureResource(2, AssetManager::RetrieveAsset<TextureAsset>(finalTexturePath)->GetTexture());

                m_FinalFramebuffer->BindShaderBufferResource(1, index, m_ObjectDataBuffer.get());

                // update transform of each cube within the buffer
                ObjectData objectData = { scene->GetEntityCachedTransform({ scene, entity }), static_cast<int>(entity), { 0.f, 0.f, 0.f } };
                m_ObjectDataBuffer->SetData(&objectData, 1, index);

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

        Renderer::Api().RenderFramebuffers(context, { m_FinalFramebuffer.get() });
    }
}