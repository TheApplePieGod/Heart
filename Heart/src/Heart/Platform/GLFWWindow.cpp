#include "hepch.h"
#include "GLFWWindow.h"

#ifndef HE_PLATFORM_ANDROID

#include "Heart/Core/App.h"
#include "Heart/Events/WindowEvents.h"
#include "Heart/Events/KeyEvents.h"
#include "Heart/Events/ButtonEvents.h"
#include "Heart/Input/Input.h"
#include "Flourish/Api/RenderContext.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

namespace Heart
{
    static void GLFWErrorCallback(int err, const char *desc)
    {
        HE_ENGINE_LOG_ERROR("GLFW Error ({0}): {1}", err, desc);
    }

    GLFWWindow::GLFWWindow(const WindowCreateInfo &createInfo)
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

        m_GLFWWindowData.DataPtr = &m_WindowData;
        glfwSetWindowUserPointer(window, &m_GLFWWindowData);

        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

        glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int width, int height)
        {
			GLFWWindowData& data = *(GLFWWindowData*)glfwGetWindowUserPointer(window);
			data.DataPtr->Width = width;
			data.DataPtr->Height = height;

			WindowResizeEvent event(width, height);
			data.DataPtr->EmitEvent(event);
        });

        glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
        {
			GLFWWindowData& data = *(GLFWWindowData*)glfwGetWindowUserPointer(window);
            KeyCode keyCode = static_cast<KeyCode>(key);

			switch (action)
			{
				case GLFW_PRESS:
				{
                    Input::AddKeyEvent(keyCode, true);
					KeyPressedEvent event(keyCode, false);
					data.DataPtr->EmitEvent(event);
				} break;
				case GLFW_RELEASE:
				{
                    Input::AddKeyEvent(keyCode, false);
					KeyReleasedEvent event(keyCode);
					data.DataPtr->EmitEvent(event);
				} break;
				case GLFW_REPEAT:
				{
                    Input::AddKeyEvent(keyCode, true);
					KeyPressedEvent event(keyCode, true);
					data.DataPtr->EmitEvent(event);
				} break;
			}
        });

        glfwSetWindowCloseCallback(window, [](GLFWwindow *window)
        {
			GLFWWindowData& data = *(GLFWWindowData*)glfwGetWindowUserPointer(window);

			WindowCloseEvent event;
			data.DataPtr->EmitEvent(event);
        });

        glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xPos, double yPos)
        {
            GLFWWindowData& data = *(GLFWWindowData*)glfwGetWindowUserPointer(window);

            bool skipDelta = false;
            if (data.MousePositionSkips > 0)
            {
                data.MousePositionSkips--;
                skipDelta = true;
            }

            Input::AddAxisEvent(AxisCode::MouseX, xPos, false, skipDelta);
            Input::AddAxisEvent(AxisCode::MouseY, yPos, false, skipDelta);
        });

        glfwSetScrollCallback(window, [](GLFWwindow *window, double xOffset, double yOffset)
        {
            Input::AddAxisEvent(AxisCode::ScrollX, xOffset, true);
            Input::AddAxisEvent(AxisCode::ScrollY, yOffset, true);
        });

        glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods)
        {
            GLFWWindowData& data = *(GLFWWindowData*)glfwGetWindowUserPointer(window);
            ButtonCode buttonCode = static_cast<ButtonCode>(button);

			switch (action)
			{
				case GLFW_PRESS:
				{
                    Input::AddButtonEvent(buttonCode, true);
					ButtonPressedEvent event(buttonCode);
					data.DataPtr->EmitEvent(event);
				} break;
				case GLFW_RELEASE:
				{
                    Input::AddButtonEvent(buttonCode, false);
					ButtonReleasedEvent event(buttonCode);
					data.DataPtr->EmitEvent(event);
				} break;
			}
        });

        // glfwSetCharCallback

        m_Window = window;
        s_WindowCount++;
    }

    GLFWWindow::~GLFWWindow()
    {
        HE_ENGINE_LOG_INFO("Destroying window ({0}x{1})", m_WindowData.Width, m_WindowData.Height);

        glfwDestroyWindow((GLFWwindow*)m_Window);

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
        glfwSetInputMode((GLFWwindow *)m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Workaround for issues with large mouse delta events occuring after reenabling the mouse cursor
        m_GLFWWindowData.MousePositionSkips = 3;
    }

    void GLFWWindow::EnableCursor()
    {
        glfwSetInputMode((GLFWwindow *)m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        double xpos, ypos;
        glfwGetCursorPos((GLFWwindow *)m_Window, &xpos, &ypos);
        Input::AddAxisEvent(AxisCode::MouseX, xpos, false, true);
        Input::AddAxisEvent(AxisCode::MouseY, xpos, false, true);
    }

    void GLFWWindow::SetWindowTitle(const char *newTitle)
    {
        m_WindowData.Title = newTitle;
        glfwSetWindowTitle((GLFWwindow *)m_Window, newTitle);
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
                glfwGetWindowPos((GLFWwindow *)m_Window, &m_SavedWindowSizeAndPosition[2], &m_SavedWindowSizeAndPosition[3]);
                glfwGetWindowSize((GLFWwindow *)m_Window, &m_SavedWindowSizeAndPosition[0], &m_SavedWindowSizeAndPosition[1]);

                // get resolution of monitor
                const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

                // switch to full screen
                glfwSetWindowMonitor((GLFWwindow *)m_Window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
            }
            else
            {
                glfwSetWindowMonitor(
                    (GLFWwindow *)m_Window,
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

    float GLFWWindow::GetDPIScale() const
    {
        int w, h;
        glfwGetFramebufferSize((GLFWwindow*)m_Window, &w, &h);

        return (float)w / m_WindowData.Width;
    }
}

#endif
