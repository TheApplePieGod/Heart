#include "htpch.h"
#include "OpenGLContext.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

namespace Heart
{
    OpenGLGraphicsPipeline* OpenGLContext::s_BoundGraphicsPipeline = nullptr;

    OpenGLContext::OpenGLContext(void* window)
    {
        m_WindowHandle = window;

        glfwMakeContextCurrent((GLFWwindow*)window);

        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		HE_ENGINE_ASSERT(status, "Failed to initialize Glad");

		HE_ENGINE_ASSERT("OpenGL Info:");
		HE_ENGINE_ASSERT("  Vendor: {0}", glGetString(GL_VENDOR));
		HE_ENGINE_ASSERT("  Renderer: {0}", glGetString(GL_RENDERER));
		HE_ENGINE_ASSERT("  Version: {0}", glGetString(GL_VERSION));

		HE_ENGINE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Heart requires at least OpenGL version 4.5");

        // opengl setup
        glEnable(GL_CULL_FACE);
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    }

    OpenGLContext::~OpenGLContext()
    {
        
    }

    void OpenGLContext::InitializeImGui()
    {
        ImGui_ImplOpenGL3_Init();
    }

    void OpenGLContext::ShutdownImGui()
    {
        ImGui_ImplOpenGL3_Shutdown();
    }

    void OpenGLContext::ImGuiBeginFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
    }

    void OpenGLContext::ImGuiEndFrame()
    {
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void OpenGLContext::BeginFrame()
    {

    }

    void OpenGLContext::EndFrame()
    {
        glfwSwapBuffers((GLFWwindow*)m_WindowHandle);
        s_BoundGraphicsPipeline = nullptr;
    }
}