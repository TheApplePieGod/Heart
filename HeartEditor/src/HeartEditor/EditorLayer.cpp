#include "htpch.h"
#include "EditorLayer.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "glm/vec4.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace HeartEditor
{
    EditorLayer::EditorLayer()
    {
        m_EditorCamera = Heart::CreateScope<EditorCamera>(70.f, 0.1f, 1000.f, 1.f);
    }

    EditorLayer::~EditorLayer()
    {
        
    }

    void EditorLayer::OnAttach()
    {
        HE_PROFILE_FUNCTION();

        SubscribeToEmitter(&EditorApp::Get().GetWindow());

        m_TestData = new TestData();

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
            m_TestData->VertexBuffer = Heart::Buffer::Create(Heart::Buffer::Type::Vertex, vertBufferLayout, (u32)vertexArray.size(), vertexArray.data());

            // index buffer
            std::vector<u32> indices = {
                0, 3, 2, 2, 1, 0, // -Z
                4, 5, 6, 6, 7, 4, // +Z
                8, 11, 10, 10, 9, 8, // +X
                12, 13, 14, 14, 15, 12, // -X
                16, 17, 18, 18, 19, 16,
                20, 23, 22, 22, 21, 20
            };
            m_TestData->IndexBuffer = Heart::Buffer::CreateIndexBuffer((u32)indices.size(), indices.data());

            // shader registry
            m_TestData->ShaderRegistry.RegisterShader("vert", "assets/shaders/main.vert", Heart::Shader::Type::Vertex);
            m_TestData->ShaderRegistry.RegisterShader("frag", "assets/shaders/main.frag", Heart::Shader::Type::Fragment);

            // texture registry
            m_TestData->TextureRegistry.RegisterTexture("fish", "assets/textures/fish.png");
            m_TestData->TextureRegistry.RegisterTexture("test", "assets/textures/test.png");

            // graphics pipeline
            Heart::GraphicsPipelineCreateInfo gpCreateInfo = {
                m_TestData->ShaderRegistry.LoadShader("vert"),
                m_TestData->ShaderRegistry.LoadShader("frag"),
                Heart::VertexTopology::TriangleList,
                vertBufferLayout,
                { { true }, { true } },
                true,
                Heart::CullMode::Backface
            };

            // per frame data buffer layout
            Heart::BufferLayout frameDataLayout = {
                { Heart::BufferDataType::Mat4 }
            };

            // per object data buffer layout
            Heart::BufferLayout objectDataLayout = {
                { Heart::BufferDataType::Mat4 },
            };

            // per frame data buffer
            glm::mat4 initialData = m_EditorCamera->GetProjectionMatrix() * m_EditorCamera->GetViewMatrix();
            m_TestData->FrameDataBuffer = Heart::Buffer::Create(Heart::Buffer::Type::Uniform, frameDataLayout, 1, &initialData);

            // object data buffer
            m_TestData->ObjectDataBuffer = Heart::Buffer::Create(Heart::Buffer::Type::Storage, objectDataLayout, 1000, nullptr);

            // framebuffer
            Heart::FramebufferCreateInfo fbCreateInfo = {
                { Heart::ColorFormat::RGBA8, { 0.f, 0.f, 0.f, 0.f } },
                { Heart::ColorFormat::RGBA8, { 0.f, 0.f, 0.f, 0.f } }
            };
            fbCreateInfo.Width = 0;
            fbCreateInfo.Height = 0;
            fbCreateInfo.SampleCount = Heart::MsaaSampleCount::None;
            fbCreateInfo.HasDepth = true;
            m_TestData->SceneFramebuffer = Heart::Framebuffer::Create(fbCreateInfo);
            m_TestData->SceneFramebuffer->RegisterGraphicsPipeline("main", gpCreateInfo);
        }

        HE_CLIENT_LOG_INFO("Editor attached");
    }

    // LOOK AT FRAMEBUFFER ATTACHMENT COLOR FORMATS
    // Buffer padding VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment

    void EditorLayer::OnUpdate(Heart::Timestep ts)
    {
        HE_PROFILE_FUNCTION();

        if (m_ViewportInput)
            m_EditorCamera->OnUpdate(ts);

        m_TestData->SceneFramebuffer->Bind();
        m_TestData->SceneFramebuffer->BindPipeline("main");

        Heart::Renderer::Api().BindVertexBuffer(*m_TestData->VertexBuffer);
        Heart::Renderer::Api().BindIndexBuffer(*m_TestData->IndexBuffer);
        
        m_TestData->FrameDataBuffer->SetData(&m_EditorCamera->GetViewProjectionMatrix(), 1, 0);

        // all shader resources must be bound before drawing
        m_TestData->SceneFramebuffer->BindShaderBufferResource(0, 0, m_TestData->FrameDataBuffer.get());
        m_TestData->SceneFramebuffer->BindShaderTextureResource(2, m_TestData->TextureRegistry.LoadTexture("test").get());
        for (u32 i = 0; i < 50; i++)
        {
            m_TestData->SceneFramebuffer->BindShaderBufferResource(1, i, m_TestData->ObjectDataBuffer.get());

            glm::vec3 objectPos = { 0.f, 0.f, 2.f + i + (i * 0.5f) };
            glm::mat4 transformed = glm::translate(glm::mat4(1.f), objectPos)
                                     * glm::scale(glm::mat4(1.f), glm::vec3(1.f, 1.f, 1.f));

            m_TestData->ObjectDataBuffer->SetData(&transformed, 1, i);

            Heart::Renderer::Api().DrawIndexed(
                m_TestData->IndexBuffer->GetAllocatedCount(),
                m_TestData->VertexBuffer->GetAllocatedCount(),
                0, 0, 1
            );
        }
        
        Heart::Renderer::Api().RenderFramebuffers(EditorApp::Get().GetWindow().GetContext(), { m_TestData->SceneFramebuffer.get() });
    }

    void EditorLayer::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin("Main Window", nullptr, windowFlags);
        
        m_Widgets.MainMenuBar.OnImGuiRender();

        ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f), 0);

        if (m_Widgets.MainMenuBar.GetWindowStatus("Viewport"))
        {
            ImGui::Begin("Viewport", m_Widgets.MainMenuBar.GetWindowStatusRef("Viewport"));

            ImVec2 viewportMin = ImGui::GetWindowContentRegionMin();
            ImVec2 viewportMax = ImGui::GetWindowContentRegionMax();
            ImVec2 viewportPos = ImGui::GetWindowPos();
            glm::vec2 viewportSize = { viewportMax.x - viewportMin.x, viewportMax.y - viewportMin.y };
            glm::vec2 viewportStart = { viewportMin.x + viewportPos.x, viewportMin.y + viewportPos.y };
            glm::vec2 viewportEnd = viewportStart + viewportSize;

            m_EditorCamera->UpdateAspectRatio(viewportSize.x / viewportSize.y);

            ImGui::GetWindowDrawList()->AddRectFilled({ viewportStart.x, viewportStart.y }, { viewportEnd.x, viewportEnd.y }, IM_COL32( 0, 0, 0, 255 )); // viewport background
            ImGui::Image(
                m_TestData->SceneFramebuffer->GetColorAttachmentImGuiHandle(0),
                { viewportSize.x, viewportSize.y },
                { 0.f, 0.f }, { 1.f, 1.f }
            );
            if (ImGui::IsItemClicked())
            {
                // disable imgui input & cursor
                m_ViewportInput = true;
                EditorApp::Get().GetWindow().DisableCursor();
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
            }
            ImGui::SetItemAllowOverlap();

            ImGui::End();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Content Browser"))
        {
            ImGui::Begin("Content Browser", m_Widgets.MainMenuBar.GetWindowStatusRef("Content Browser"));

            ImGui::End();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Properties Panel"))
        {
            ImGui::Begin("Properties Panel", m_Widgets.MainMenuBar.GetWindowStatusRef("Properties Panel"));
            ImGui::End();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Scene Hierarchy"))
        {
            ImGui::Begin("Scene Hierarchy", m_Widgets.MainMenuBar.GetWindowStatusRef("Scene Hierarchy"));
            ImGui::End();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("Settings"))
        {
            ImGui::Begin("Settings", m_Widgets.MainMenuBar.GetWindowStatusRef("Settings"));
            ImGui::End();
        }

        if (m_Widgets.MainMenuBar.GetWindowStatus("ImGui Demo"))
        {
            ImGui::ShowDemoWindow(m_Widgets.MainMenuBar.GetWindowStatusRef("ImGui Demo"));
        }

        ImGui::End();

        ImGui::PopStyleVar(2);
    }

    void EditorLayer::OnDetach()
    {
        UnsubscribeFromEmitter(&EditorApp::Get().GetWindow());
        delete m_TestData;
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
        else if (event.GetKeyCode() == Heart::KeyCode::F11)
            EditorApp::Get().GetWindow().ToggleFullscreen();
        
        return true;
    }
}