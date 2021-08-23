#include "htpch.h"
#include "EditorLayer.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "imgui/imgui.h"
#include "glm/vec4.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace HeartEditor
{
    EditorLayer::EditorLayer()
    {
        m_EditorCamera = Heart::CreateScope<EditorCamera>(70.f, 0.1f, 1000.f, 1.f);
        SubscribeToEmitter(&EditorApp::Get().GetWindow());
    }

    EditorLayer::~EditorLayer()
    {
        UnsubscribeFromEmitter(&EditorApp::Get().GetWindow());
    }

    void EditorLayer::OnAttach()
    {
        HE_CLIENT_LOG_INFO("Editor attached");

        // create a test scene
        {
            // vertex buffer
            TestData::Vertex vertexArray[4] = {
                { { -0.5f, -0.5f, 0.f }, { 0.f, 1.f } },
                { { 0.5f, -0.5f, 0.f }, { 1.f, 1.f } },
                { { 0.5f, 0.7f, 0.f }, { 1.f, 0.f } },
                { { -0.5f, 0.5f, 0.f }, { 0.f, 0.f } }
            };
            Heart::BufferLayout vertBufferLayout = {
                { Heart::BufferDataType::Float3 },
                { Heart::BufferDataType::Float2 }
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

            // texture registry
            m_TestData.TextureRegistry.RegisterTexture("fish", "assets/textures/fish.png");

            // shader input set
            m_TestData.ShaderInputSet = Heart::ShaderInputSet::Create({
                { Heart::ShaderInputType::Buffer, Heart::ShaderBindType::Vertex, 0 },
                { Heart::ShaderInputType::Buffer, Heart::ShaderBindType::Vertex, 1 },
                { Heart::ShaderInputType::Texture, Heart::ShaderBindType::Fragment, 2 }
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
                Heart::CullMode::Backface
            };

            // per frame data buffer layout
            Heart::BufferLayout frameDataLayout = {
                { Heart::BufferDataType::Mat4 }
            };

            // per object data buffer layout
            Heart::BufferLayout objectDataLayout = {
                { Heart::BufferDataType::Mat4 }
            };

            // per frame data buffer
            glm::mat4 initialData = m_EditorCamera->GetProjectionMatrix() * m_EditorCamera->GetViewMatrix();
            m_TestData.FrameDataBuffer = Heart::Buffer::Create(frameDataLayout, 1, &initialData);

            // object data buffer
            glm::vec3 objectPos = { 0.f, 0.f, 2.f };
            glm::mat4 initialData2 = glm::translate(glm::mat4(1.f), objectPos)
                                     * glm::scale(glm::mat4(1.f), glm::vec3(1.f, 1.f, 1.f));
            m_TestData.ObjectDataBuffer = Heart::Buffer::Create(objectDataLayout, 1, &initialData2);

            // framebuffer
            Heart::FramebufferCreateInfo fbCreateInfo = {
                { true, Heart::ColorFormat::RGBA8, { 0.f, 0.f, 0.f, 0.f } },
                { false, Heart::ColorFormat::RGBA8, { 0.f, 0.f, 0.f, 0.f } }
            };
            fbCreateInfo.Width = 0;
            fbCreateInfo.Height = 0;
            fbCreateInfo.SampleCount = Heart::MsaaSampleCount::None;
            m_TestData.SceneFramebuffer = Heart::Framebuffer::Create(fbCreateInfo);
            m_TestData.SceneFramebuffer->RegisterGraphicsPipeline("main", gpCreateInfo);
        }
    }

    // LOOK AT FRAMEBUFFER ATTACHMENT COLOR FORMATS
    // Framebuffer multiple binds per frame
    // Buffer padding VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment

    void EditorLayer::OnUpdate(Heart::Timestep ts)
    {
        if (m_ViewportInput)
            m_EditorCamera->OnUpdate(ts);

        m_TestData.SceneFramebuffer->Bind();
        
        Heart::Renderer::Api().BindVertexBuffer(*m_TestData.VertexBuffer);
        Heart::Renderer::Api().BindIndexBuffer(*m_TestData.IndexBuffer);

        m_TestData.SceneFramebuffer->BindPipeline("main");

        Heart::ShaderInputBindPoint bindPoint = m_TestData.ShaderInputSet->CreateBindPoint({
            { m_TestData.FrameDataBuffer, nullptr },
            { m_TestData.ObjectDataBuffer, nullptr },
            { nullptr, m_TestData.TextureRegistry.LoadTexture("fish") }
        });
        m_TestData.SceneFramebuffer->BindShaderInputSet(bindPoint, 0);
        m_TestData.FrameDataBuffer->SetData(&m_EditorCamera->GetViewProjectionMatrix(), 1, 0);

        Heart::Renderer::Api().DrawIndexed(
            m_TestData.IndexBuffer->GetAllocatedCount(),
            m_TestData.VertexBuffer->GetAllocatedCount(),
            0, 0, 1
        );

        m_TestData.SceneFramebuffer->Submit(EditorApp::Get().GetWindow().GetContext());
    }

    void EditorLayer::OnImGuiRender()
    {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin("Heart Editor", nullptr, windowFlags);
        
        ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f), 0);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Begin("Viewport");

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		glm::vec2 ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

        m_EditorCamera->UpdateAspectRatio(viewportPanelSize.x / viewportPanelSize.y);

        // ImGui::Image(
        //     m_TestData.SceneFramebuffer->GetRawAttachmentImageHandle(0, Heart::FramebufferAttachmentType::Color),
        //     { (f32)m_TestData.SceneFramebuffer->GetWidth(), (f32)m_TestData.SceneFramebuffer->GetHeight() }
        // );
        // ImGui::SameLine();
        ImGui::Image(
            m_TestData.SceneFramebuffer->GetRawAttachmentImageHandle(1, Heart::FramebufferAttachmentType::Color),
            { ViewportSize.x, ViewportSize.y },
            { 0.f, 0.f }, { 1.f, 1.f }
        );
        
        if (ImGui::IsItemClicked())
        {
            // disable imgui input & cursor
            m_ViewportInput = true;
            EditorApp::Get().GetWindow().DisableCursor();
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
        }

        ImGui::End();
        ImGui::PopStyleColor();

        ImGui::Begin("Content Browser");
        ImGui::End();

        ImGui::Begin("Properties Panel");
        ImGui::End();

        ImGui::Begin("Scene Hierarchy");
        ImGui::End();

        ImGui::Begin("Settings");
        ImGui::End();

        ImGui::End();
    }

    void EditorLayer::OnDetach()
    {

    }

    void EditorLayer::OnEvent(Heart::Event& event)
    {
        event.Map<Heart::KeyPressedEvent>(HE_BIND_EVENT_FN(EditorLayer::KeyPressedEvent));
    }

    bool EditorLayer::KeyPressedEvent(Heart::KeyPressedEvent& event)
    {
        if (event.GetKeyCode() == Heart::KeyCode::Escape)
        {
            if (m_ViewportInput)
            {
                // enable imgui input & cursor
                m_ViewportInput = false;
                EditorApp::Get().GetWindow().EnableCursor();
                ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            }
            else
                EditorApp::Get().Close();
        }
        
        return true;
    }
}