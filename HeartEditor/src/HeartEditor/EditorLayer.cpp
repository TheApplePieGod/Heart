#include "htpch.h"
#include "EditorLayer.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "imgui/imgui.h"
#include "glm/vec4.hpp"

namespace HeartEditor
{
    EditorLayer::EditorLayer()
    {
        m_EditorCamera = Heart::CreateScope<EditorCamera>(70.f, 0.1f, 1000.f, 1.f);
    }

    void EditorLayer::OnAttach()
    {
        HE_CLIENT_LOG_INFO("Editor attached");

        // create a test scene
        {
            // vertex buffer
            f32 vertexArray[3 * 4] = {
                -0.5f, -0.5f, 0.f,
                0.5f, -0.5f, 0.f,
                0.5f, 0.5f, 0.f,
                -0.5f, 0.5f, 0.f
            };
            Heart::BufferLayout vertBufferLayout = {
                { Heart::BufferDataType::Float3 }
            };
            m_TestData.VertexBuffer = Heart::VertexBuffer::Create(vertBufferLayout, 4, vertexArray);

            // index buffer
            u32 indices[] = {
                0, 3, 2, 2, 1, 0
            };
            m_TestData.IndexBuffer = Heart::IndexBuffer::Create(6, indices);

            // shader registry
            m_TestData.ShaderRegistry.RegisterShader("vert", "assets/shaders/main.vert", Heart::Shader::Type::Vertex);
            m_TestData.ShaderRegistry.RegisterShader("frag", "assets/shaders/main.frag", Heart::Shader::Type::Fragment);

            // shader input set
            m_TestData.ShaderInputSet = Heart::ShaderInputSet::Create({
                { Heart::ShaderInputType::Buffer, Heart::ShaderBindType::Fragment, 0 }
            });

            // graphics pipeline
            Heart::GraphicsPipelineCreateInfo gpCreateInfo = {
                m_TestData.ShaderRegistry.LoadShader("vert"),
                m_TestData.ShaderRegistry.LoadShader("frag"),
                Heart::VertexTopology::TriangleList,
                vertBufferLayout,
                { { true }, { true } },
                { m_TestData.ShaderInputSet },
                true,
                Heart::CullMode::None
            };

            // per frame data buffer layout
            Heart::BufferLayout frameDataLayout = {
                { Heart::BufferDataType::Float4 }
            };

            // per frame data buffer
            glm::vec4 initialData = { 1.f, 1.f, 0.f, 1.f };
            m_TestData.FrameDataBuffer = Heart::Buffer::Create(frameDataLayout, 1, &initialData);

            // framebuffer
            Heart::FramebufferCreateInfo fbCreateInfo = {
                { true, Heart::ColorFormat::RGBA8, { 0.f, 1.f, 0.f, 1.f } },
                { false, Heart::ColorFormat::RGBA8, { 0.f, 1.f, 0.f, 1.f } }
            };
            fbCreateInfo.Width = 500;
            fbCreateInfo.Height = 500;
            fbCreateInfo.SampleCount = Heart::MsaaSampleCount::None;
            m_TestData.SceneFramebuffer = Heart::Framebuffer::Create(fbCreateInfo);
            m_TestData.SceneFramebuffer->RegisterGraphicsPipeline("main", gpCreateInfo);
        }
    }

    void EditorLayer::OnUpdate()
    {
        m_TestData.SceneFramebuffer->Bind();
        
        Heart::Renderer::Api().BindVertexBuffer(*m_TestData.VertexBuffer);
        Heart::Renderer::Api().BindIndexBuffer(*m_TestData.IndexBuffer);

        m_TestData.SceneFramebuffer->BindPipeline("main");

        for (int i = 0; i < 1; i++)
        {
            Heart::ShaderInputBindPoint bindPoint = m_TestData.ShaderInputSet->CreateBindPoint({
                { m_TestData.FrameDataBuffer, nullptr }
            });
            m_TestData.SceneFramebuffer->BindShaderInputSet(bindPoint, 0);
        }

        glm::vec4 newColor = { rand() % 100 / 100.f, rand() % 100 / 100.f, rand() % 100 / 100.f, 1.f };
        m_TestData.FrameDataBuffer->SetData(&newColor, 1, 0);

        Heart::Renderer::Api().DrawIndexed(
            m_TestData.IndexBuffer->GetAllocatedCount(),
            m_TestData.VertexBuffer->GetAllocatedCount(),
            0, 0, 1
        );

        m_TestData.SceneFramebuffer->Submit(EditorApp::Get().GetWindow().GetContext());
    }

    void EditorLayer::OnImGuiRender()
    {
        //ImGui::ShowDemoWindow();
        ImGui::Begin("Test");
        ImGui::Image(
            m_TestData.SceneFramebuffer->GetRawAttachmentImageHandle(1, Heart::FramebufferAttachmentType::Color),
            { (f32)m_TestData.SceneFramebuffer->GetWidth(), (f32)m_TestData.SceneFramebuffer->GetHeight() }
        );

        ImGui::End();
    }

    void EditorLayer::OnDetach()
    {

    }

    EditorLayer::~EditorLayer()
    {
        
    }
}