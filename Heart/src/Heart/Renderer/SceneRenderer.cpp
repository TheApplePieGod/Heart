#include "htpch.h"
#include "SceneRenderer.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Entity.h"
#include "glm/gtc/matrix_transform.hpp"

namespace Heart
{
    SceneRenderer::SceneRenderer()
    {
        // vertex buffer
        std::vector<Vertex> vertexArray = {
            { { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f } }, // -Z
            { { 0.5f, -0.5f, -0.5f }, { 1.f, 1.f } },
            { { 0.5f, 0.5f, -0.5f }, { 1.f, 0.f } },
            { { -0.5f, 0.5f, -0.5f }, { 0.f, 0.f } },

            { { -0.5f, -0.5f, 0.5f }, { 0.f, 1.f } }, // +Z
            { { 0.5f, -0.5f, 0.5f }, { 1.f, 1.f } },
            { { 0.5f, 0.5f, 0.5f }, { 1.f, 0.f } },
            { { -0.5f, 0.5f, 0.5f }, { 0.f, 0.f } },

            { { 0.5f, -0.5f, -0.5f }, { 0.f, 1.f } }, // +X
            { { 0.5f, -0.5f, 0.5f }, { 1.f, 1.f } },
            { { 0.5f, 0.5f, 0.5f }, { 1.f, 0.f } },
            { { 0.5f, 0.5f, -0.5f }, { 0.f, 0.f } },

            { { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f } }, // -X
            { { -0.5f, -0.5f, 0.5f }, { 1.f, 1.f } },
            { { -0.5f, 0.5f, 0.5f }, { 1.f, 0.f } },
            { { -0.5f, 0.5f, -0.5f }, { 0.f, 0.f } },

            { { -0.5f, 0.5f, -0.5f }, { 0.f, 1.f } }, // +Y
            { { -0.5f, 0.5f, 0.5f }, { 0.f, 0.f } },
            { { 0.5f, 0.5f, 0.5f }, { 1.f, 0.f } },
            { { 0.5f, 0.5f, -0.5f }, { 1.f, 1.f } },

            { { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f } }, // -Y
            { { -0.5f, -0.5f, 0.5f }, { 0.f, 0.f } },
            { { 0.5f, -0.5f, 0.5f }, { 1.f, 0.f } },
            { { 0.5f, -0.5f, -0.5f }, { 1.f, 1.f } }
        };

        BufferLayout vertBufferLayout = {
            { BufferDataType::Float3 },
            { BufferDataType::Float2 }
        };
        m_VertexBuffer = Buffer::Create(Buffer::Type::Vertex, vertBufferLayout, (u32)vertexArray.size(), vertexArray.data());

        // index buffer
        std::vector<u32> indices = {
            0, 3, 2, 2, 1, 0, // -Z
            4, 5, 6, 6, 7, 4, // +Z
            8, 11, 10, 10, 9, 8, // +X
            12, 13, 14, 14, 15, 12, // -X
            16, 17, 18, 18, 19, 16,
            20, 23, 22, 22, 21, 20
        };
        m_IndexBuffer = Buffer::CreateIndexBuffer((u32)indices.size(), indices.data());

        // shader registry
        m_ShaderRegistry.RegisterShader("vert", "assets/shaders/main.vert", Shader::Type::Vertex);
        m_ShaderRegistry.RegisterShader("frag", "assets/shaders/main.frag", Shader::Type::Fragment);

        // texture registry
        m_TextureRegistry.RegisterTexture("fish", "assets/textures/fish.png");
        m_TextureRegistry.RegisterTexture("test", "assets/textures/test.png");

        // graphics pipeline
        GraphicsPipelineCreateInfo gpCreateInfo = {
            m_ShaderRegistry.LoadShader("vert"),
            m_ShaderRegistry.LoadShader("frag"),
            VertexTopology::TriangleList,
            vertBufferLayout,
            { { true }, { false } },
            true,
            CullMode::Backface
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
        m_FrameDataBuffer = Buffer::Create(Buffer::Type::Uniform, frameDataLayout, 1, nullptr);

        // object data buffer
        m_ObjectDataBuffer = Buffer::Create(Buffer::Type::Storage, objectDataLayout, 1000, nullptr);

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
        HE_ENGINE_ASSERT(scene, "Scene cannot be nullptr");

        m_FinalFramebuffer->Bind();
        m_FinalFramebuffer->BindPipeline("main");

        Renderer::Api().BindVertexBuffer(*m_VertexBuffer);
        Renderer::Api().BindIndexBuffer(*m_IndexBuffer);
        
        m_FrameDataBuffer->SetData(&viewProjection, 1, 0);

        // all shader resources must be bound before drawing
        m_FinalFramebuffer->BindShaderBufferResource(0, 0, m_FrameDataBuffer.get());
        m_FinalFramebuffer->BindShaderTextureResource(2, m_TextureRegistry.LoadTexture("test").get());

        auto group = scene->GetRegistry().group<TransformComponent, MeshComponent>();
        u32 index = 0;
        for (auto entity : group)
        {
            auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

            m_FinalFramebuffer->BindShaderBufferResource(1, index, m_ObjectDataBuffer.get());

            // update transform of each cube within the buffer
            ObjectData objectData = { transform.GetTransformMatrix(), static_cast<int>(entity), { 0.f, 0.f, 0.f } };
            m_ObjectDataBuffer->SetData(&objectData, 1, index);

            // draw
            Renderer::Api().DrawIndexed(
                m_IndexBuffer->GetAllocatedCount(),
                m_VertexBuffer->GetAllocatedCount(),
                0, 0, 1
            );

            index++;
        }
        
        Renderer::Api().RenderFramebuffers(context, { m_FinalFramebuffer.get() });
    }
}