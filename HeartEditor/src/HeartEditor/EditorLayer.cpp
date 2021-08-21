#include "htpch.h"
#include "EditorLayer.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
    void EditorLayer::OnAttach()
    {
        HE_CLIENT_LOG_WARN("Editor attached");

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

            // graphics pipeline
            Heart::GraphicsPipelineCreateInfo gpCreateInfo = {
                m_TestData.ShaderRegistry.LoadShader("vert"),
                m_TestData.ShaderRegistry.LoadShader("frag"),
                Heart::VertexTopology::TriangleList,
                vertBufferLayout,
                true,
                Heart::CullMode::None
            };

            // framebuffer
            Heart::FrameBufferCreateInfo fbCreateInfo = {
                { true, Heart::ColorFormat::RGBA8, { 0.f, 1.f, 0.f, 1.f } }
            };
            fbCreateInfo.Width = 1920;
            fbCreateInfo.Height = 1080;
            fbCreateInfo.SampleCount = Heart::MsaaSampleCount::None;
            m_TestData.SceneFrameBuffer = Heart::FrameBuffer::Create(fbCreateInfo);
            m_TestData.SceneFrameBuffer->RegisterGraphicsPipeline("main", gpCreateInfo);
        }
    }

    void EditorLayer::OnUpdate()
    {
        m_TestData.SceneFrameBuffer->Bind();
        
        Heart::Renderer::Api().BindVertexBuffer(*m_TestData.VertexBuffer);
        Heart::Renderer::Api().BindIndexBuffer(*m_TestData.IndexBuffer);

        m_TestData.SceneFrameBuffer->BindPipeline("main");

        Heart::Renderer::Api().DrawIndexed(
            m_TestData.IndexBuffer->GetAllocatedCount(),
            m_TestData.VertexBuffer->GetAllocatedCount(),
            0, 0, 1
        );

        m_TestData.SceneFrameBuffer->Submit(EditorApp::Get().GetWindow().GetContext());
    }

    void EditorLayer::OnImGuiRender()
    {
        //ImGui::ShowDemoWindow();
        ImGui::Begin("Test");
        ImGui::Image(
            m_TestData.SceneFrameBuffer->GetRawAttachmentImageHandle(0, Heart::FrameBufferAttachmentType::Color),
            { 500, 500 }
        );

        ImGui::End();
    }

    void EditorLayer::OnDetach()
    {

    }
}