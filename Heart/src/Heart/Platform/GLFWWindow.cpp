#include "hepch.h"
#include "GLFWWindow.h"

#ifndef HE_PLATFORM_ANDROID

#include "Heart/Events/WindowEvents.h"
#include "Heart/Events/KeyboardEvents.h"
#include "Heart/Events/MouseEvents.h"
#include "Heart/Input/Input.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "Flourish/Api/RenderContext.h"

namespace Heart
{
    static void GLFWErrorCallback(int err, const char* desc)
    {
        HE_ENGINE_LOG_ERROR("GLFW Error ({0}): {1}", err, desc);
    }

    GLFWWindow::GLFWWindow(const WindowCreateInfo& createInfo)
        : Window(createInfo)
    {
        if (s_WindowCount == 0)
        {
            int success = glfwInit();
            HE_ENGINE_ASSERT(success, "Failed to initialize GLFW");
            glfwSetErrorCallback(GLFWErrorCallback);
        }

        if (Flourish::Context::BackendType() == Flourish::BackendType::Vulkan)
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            HE_ENGINE_ASSERT(glfwVulkanSupported(), "Device does not support vulkan rendering");
        }

        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        auto window = glfwCreateWindow(createInfo.Width, createInfo.Height, createInfo.Title.Data(), nullptr, nullptr);

        Flourish::RenderContextCreateInfo ctxCreateInfo;
        ctxCreateInfo.Window = window;
        ctxCreateInfo.Width = createInfo.Width;
        ctxCreateInfo.Height = createInfo.Height;
        m_RenderContext = Flourish::RenderContext::Create(ctxCreateInfo);

        glfwSetWindowUserPointer(window, &m_WindowData);
        
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

        glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EmitEvent(event);
		});

        glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
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

        glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			WindowCloseEvent event;
			data.EmitEvent(event);
		});

        glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xPos, double yPos)
        {
            Input::UpdateMousePosition(xPos, yPos);
        });

        glfwSetScrollCallback(window, [](GLFWwindow* window, double xOffset, double yOffset)
        {
            Input::UpdateScrollOffset(xOffset, yOffset);
        });

        glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            MouseCode mouseCode = static_cast<MouseCode>(button);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(mouseCode);
					data.EmitEvent(event);
				} break;
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(mouseCode);
					data.EmitEvent(event);
				} break;
			}
        });

        //glfwSetCharCallback

        m_Window = window;
        s_WindowCount++;
    }

    GLFWWindow::~GLFWWindow()
    {
        // This is the last window
        if (s_WindowCount == 1)
            glfwTerminate();
    }

    bool GLFWWindow::PollEvents()
    {
        HE_PROFILE_FUNCTION();
        
        glfwPollEvents();

        return false;
    }

    void GLFWWindow::EndFrame()
    {
        Flourish::Context::PushFrameRenderContext(m_RenderContext.get());
    }

    void GLFWWindow::DisableCursor()
    {
        glfwSetInputMode((GLFWwindow*)m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void GLFWWindow::EnableCursor()
    {
        glfwSetInputMode((GLFWwindow*)m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        
        double xpos, ypos;
        glfwGetCursorPos((GLFWwindow*)m_Window, &xpos, &ypos);
        Input::SetMousePosition(xpos, ypos);
    }

    void GLFWWindow::SetWindowTitle(const char* newTitle)
    {
        m_WindowData.Title = newTitle;
        glfwSetWindowTitle((GLFWwindow*)m_Window, newTitle);
    }

    void GLFWWindow::SetFullscreen(bool fullscreen)
    {
        bool fullscreenStatus = IsFullscreen();
        if (fullscreenStatus == fullscreen)
            return;
        else
        {
            if (fullscreen)
            {
                // backup window position and window size
                glfwGetWindowPos((GLFWwindow*)m_Window, &m_SavedWindowSizeAndPosition[2], &m_SavedWindowSizeAndPosition[3] );
                glfwGetWindowSize((GLFWwindow*)m_Window, &m_SavedWindowSizeAndPosition[0], &m_SavedWindowSizeAndPosition[1] );

                // get resolution of monitor
                const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

                // switch to full screen
                glfwSetWindowMonitor((GLFWwindow*)m_Window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
            }
            else
            {
                glfwSetWindowMonitor(
                    (GLFWwindow*)m_Window,
                    nullptr,
                    m_SavedWindowSizeAndPosition[2],
                    m_SavedWindowSizeAndPosition[3],
                    m_SavedWindowSizeAndPosition[0],
                    m_SavedWindowSizeAndPosition[1],
                    0
                );
            }
        }
    }

    void GLFWWindow::ToggleFullscreen()
    {
        SetFullscreen(!IsFullscreen());
    }

    bool GLFWWindow::IsFullscreen() const
    {
        return glfwGetWindowMonitor((GLFWwindow*)m_Window) != nullptr;
    }
}

#endif
