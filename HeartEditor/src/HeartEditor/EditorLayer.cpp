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
            std::vector<TestData::Vertex> vertexArray = {
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
            Heart::BufferLayout vertBufferLayout = {
                { Heart::BufferDataType::Float3 },
                { Heart::BufferDataType::Float2 }
            };
            m_TestData.VertexBuffer = Heart::VertexBuffer::Create(vertBufferLayout, (u32)vertexArray.size(), vertexArray.data());

            // index buffer
            std::vector<u32> indices = {
                0, 3, 2, 2, 1, 0, // -Z
                4, 5, 6, 6, 7, 4, // +Z
                8, 11, 10, 10, 9, 8, // +X
                12, 13, 14, 14, 15, 12, // -X
                16, 17, 18, 18, 19, 16,
                20, 23, 22, 22, 21, 20
            };
            m_TestData.IndexBuffer = Heart::IndexBuffer::Create((u32)indices.size(), indices.data());

            // shader registry
            m_TestData.ShaderRegistry.RegisterShader("vert", "assets/shaders/main.vert", Heart::Shader::Type::Vertex);
            m_TestData.ShaderRegistry.RegisterShader("frag", "assets/shaders/main.frag", Heart::Shader::Type::Fragment);

            // texture registry
            m_TestData.TextureRegistry.RegisterTexture("fish", "assets/textures/fish.png");
            m_TestData.TextureRegistry.RegisterTexture("test", "assets/textures/test.png");

            // shader input set
            m_TestData.ShaderInputSet = Heart::ShaderInputSet::Create({
                { Heart::ShaderInputType::Buffer, Heart::ShaderBindType::Vertex, 0 },
                { Heart::ShaderInputType::BigBuffer, Heart::ShaderBindType::Vertex, 1 },
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
            m_TestData.ObjectDataBuffer = Heart::BigBuffer::Create(objectDataLayout, 1000, nullptr);

            // framebuffer
            Heart::FramebufferCreateInfo fbCreateInfo = {
                { Heart::ColorFormat::RGBA8, { 0.f, 0.f, 0.f, 0.f } },
                { Heart::ColorFormat::RGBA8, { 0.f, 0.f, 0.f, 0.f } }
            };
            fbCreateInfo.Width = 0;
            fbCreateInfo.Height = 0;
            fbCreateInfo.SampleCount = Heart::MsaaSampleCount::None;
            fbCreateInfo.HasDepth = true;
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
            { nullptr, m_TestData.TextureRegistry.LoadTexture("test") }
        });
        
        m_TestData.FrameDataBuffer->SetData(&m_EditorCamera->GetViewProjectionMatrix(), 1, 0);

        for (u32 i = 0; i < 50; i++)
        {
            m_TestData.SceneFramebuffer->BindShaderInputSet(bindPoint, 0, { 0, i * (u32)sizeof(glm::mat4) });

            glm::vec3 objectPos = { 0.f, 0.f, 2.f + i + (i * 0.5f) };
            glm::mat4 transformed = glm::translate(glm::mat4(1.f), objectPos)
                                     * glm::scale(glm::mat4(1.f), glm::vec3(1.f, 1.f, 1.f));

            m_TestData.ObjectDataBuffer->SetData(&transformed, 1, i);

            Heart::Renderer::Api().DrawIndexed(
                m_TestData.IndexBuffer->GetAllocatedCount(),
                m_TestData.VertexBuffer->GetAllocatedCount(),
                0, 0, 1
            );
        }
        
        Heart::Renderer::Api().RenderFramebuffers(EditorApp::Get().GetWindow().GetContext(), { m_TestData.SceneFramebuffer.get() });
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
            m_TestData.SceneFramebuffer->GetColorAttachmentImGuiHandle(0),
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