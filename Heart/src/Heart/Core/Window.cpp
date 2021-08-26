#include "htpch.h"
#include "Window.h"

#include "Heart/Events/WindowEvents.h"
#include "Heart/Events/KeyboardEvents.h"
#include "Heart/Input/Input.h"
#include "Heart/Renderer/Renderer.h"

namespace Heart
{
    int Window::s_WindowCount = 0;
    Ref<Window> Window::s_MainWindow = nullptr;

    static void GLFWErrorCallback(int err, const char* desc)
    {
        HE_ENGINE_LOG_ERROR("GLFW Error ({0}): {1}", err, desc);
    }

    Ref<Window> Window::Create(const WindowSettings& settings)
    {
        HE_ENGINE_LOG_INFO("Creating window ({0}x{1})", settings.Width, settings.Height);
        return CreateRef<Window>(settings);
    }

    Window::Window(const WindowSettings& settings)
    {
        m_WindowData.Title = settings.Title;
        m_WindowData.Width = settings.Width;
        m_WindowData.Height = settings.Height;
        m_WindowData.EmitEvent = HE_BIND_EVENT_FN(Window::Emit);

        if (s_WindowCount == 0)
        {
            int success = glfwInit();
            HE_ENGINE_ASSERT(success, "Failed to initialize GLFW");
            glfwSetErrorCallback(GLFWErrorCallback);
        }

        if (Renderer::GetApiType() == RenderApi::Type::Vulkan)
            HE_ENGINE_ASSERT(glfwVulkanSupported(), "Device does not support vulkan rendering");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_Window = glfwCreateWindow(settings.Width, settings.Height, settings.Title.c_str(), nullptr, nullptr);
        s_WindowCount++;

        m_GraphicsContext = GraphicsContext::Create(m_Window);

        glfwSetWindowUserPointer(m_Window, &m_WindowData);

        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EmitEvent(event);
		});

        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            KeyCode keyCode = static_cast<KeyCode>(key);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(keyCode, false);
					data.EmitEvent(event);
				} break;
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(keyCode);
					data.EmitEvent(event);
				} break;
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(keyCode, true);
					data.EmitEvent(event);
				} break;
			}
		});

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EmitEvent(event);
		});

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            
            Input::UpdateMousePosition(xPos, yPos);
        });

        //glfwSetCharCallback
        //glfwSetMouseButtonCallback
        //glfwSetScrollCallback
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_Window);
        s_WindowCount--;

        if (s_WindowCount == 0)
            glfwTerminate();
    }

    void Window::BeginFrame()
    {
        m_GraphicsContext->BeginFrame();
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    void Window::EndFrame()
    {
        m_GraphicsContext->EndFrame();
        Input::ClearMouseDelta();
    }

    void Window::DisableCursor()
    {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void Window::EnableCursor()
    {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}