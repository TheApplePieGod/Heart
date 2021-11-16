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
        AssetManager::RegisterAsset(Asset::Type::Mesh, "assets/meshes/sponza/sponza.gltf");

        // graphics pipeline
        GraphicsPipelineCreateInfo gpCreateInfo = {
            "assets/shaders/main.vert",
            "assets/shaders/main.frag",
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { true }, { false } },
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
        FramebufferCreateInfo fbCreateInfo = {
            { { 0.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::RGBA8, false },
            { { -1.f, 0.f, 0.f, 0.f }, Heart::ColorFormat::R32F, true }
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

        auto& meshAsset = AssetManager::RetrieveAsset<MeshAsset>("assets/meshes/sponza/sponza.gltf")->GetSubmesh(0);
        Renderer::Api().BindVertexBuffer(*meshAsset.GetVertexBuffer());
        Renderer::Api().BindIndexBuffer(*meshAsset.GetIndexBuffer());

        m_FrameDataBuffer->SetData(&viewProjection, 1, 0);

        // all shader resources must be bound before drawing
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());
        m_FinalFramebuffer->BindShaderTextureResource(2, AssetManager::RetrieveAsset<TextureAsset>("assets/textures/test.png")->GetTexture());

        auto group = scene->GetRegistry().group<TransformComponent, MeshComponent>();
        u32 index = 0;
        for (auto entity : group)
        {
            auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

            m_FinalFramebuffer->BindShaderBufferResource(1, index, m_ObjectDataBuffer.get());

            // update transform of each cube within the buffer
            ObjectData objectData = { scene->GetEntityCachedTransform({ scene, entity }), static_cast<int>(entity), { 0.f, 0.f, 0.f } };
            m_ObjectDataBuffer->SetData(&objectData, 1, index);

            // draw
            Renderer::Api().DrawIndexed(
                meshAsset.GetIndexBuffer()->GetAllocatedCount(),
                meshAsset.GetVertexBuffer()->GetAllocatedCount(),
                0, 0, 1
            );

            index++;
        }
        
        Renderer::Api().RenderFramebuffers(context, { m_FinalFramebuffer.get() });
    }
}