#include "hepch.h"
#include "OpenGLContext.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/OpenGL/OpenGLFramebuffer.h"
#include "Heart/Core/Timing.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

namespace Heart
{
    static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
    {
        if (id == 131154) return; // pixel-path performance warning
        if (id == 131186) return; // buffer gpu->cpu transfer performance warning

        switch (severity)
        {
            default:
                break;
            //case GL_DEBUG_SEVERITY_NOTIFICATION: HE_ENGINE_LOG_TRACE("OpenGL Debug: {0}", message); break;
            //case GL_DEBUG_SEVERITY_LOW: HE_ENGINE_LOG_INFO("OpenGL Debug: {0}", message); break;
            case GL_DEBUG_SEVERITY_MEDIUM: HE_ENGINE_LOG_WARN("OpenGL Debug: {0}", message); break;
            case GL_DEBUG_SEVERITY_HIGH: HE_ENGINE_LOG_ERROR("OpenGL Debug: {0}", message); break;
        }
    }

    OpenGLContext::OpenGLContext(void* window)
    {
        m_WindowHandle = window;

        glfwMakeContextCurrent((GLFWwindow*)window);

        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		HE_ENGINE_ASSERT(status, "Failed to initialize Glad");

		HE_ENGINE_LOG_INFO("OpenGL Info:");
		HE_ENGINE_LOG_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
		HE_ENGINE_LOG_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
		HE_ENGINE_LOG_INFO("  Version: {0}", glGetString(GL_VERSION));

		HE_ENGINE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Heart requires at least OpenGL version 4.5");

        // opengl setup
        glEnable(GL_MULTISAMPLE);
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);  

        #ifdef HE_DEBUG
            glEnable(GL_DEBUG_OUTPUT);
            glDebugMessageCallback(debugCallback, nullptr);
        #endif

        glGetIntegerv(GL_MAX_SAMPLES, &s_MsaaMaxSamples);
    }

    OpenGLContext::~OpenGLContext()
    {
        ProcessJobQueue();
    }

    void OpenGLContext::InitializeImGui()
    {
        ImGui_ImplOpenGL3_Init();
    }

    void OpenGLContext::ShutdownImGui()
    {
        ProcessJobQueue();
        ImGui_ImplOpenGL3_Shutdown();
    }

    void OpenGLContext::ImGuiBeginFrame()
    {
        HE_PROFILE_FUNCTION();
        
        ImGui_ImplOpenGL3_NewFrame();
    }

    void OpenGLContext::ImGuiEndFrame()
    {
        HE_PROFILE_FUNCTION();

        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void OpenGLContext::BeginFrame()
    {
        HE_PROFILE_FUNCTION();

        ProcessJobQueue();
    }

    void OpenGLContext::EndFrame()
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("OpenGLContext::EndFrame");

        glfwSwapBuffers((GLFWwindow*)m_WindowHandle);
        s_BoundFramebuffer = nullptr;
    }

    void OpenGLContext::ProcessJobQueue()
    {
        auto& queue = Renderer::GetJobQueue();
        while (!queue.empty())
        {
            Renderer::LockJobQueue();
            auto job = queue.front();
            queue.pop_front();
            Renderer::UnlockJobQueue();

			job(); // Run the job
		}
    }
}