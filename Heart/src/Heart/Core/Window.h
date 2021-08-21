#pragma once

#include "Heart/Events/EventEmitter.h"
#include "Heart/Renderer/GraphicsContext.h"
#include "GLFW/glfw3.h"

namespace Heart
{
    struct WindowSettings
    {
        std::string Title;
        u32 Width, Height;

        WindowSettings(const std::string& title = "Heart Engine",
			        u32 width = 1920,
			        u32 height = 1080)
			: Title(title), Width(width), Height(height)
		{}
    };

    struct WindowData
    {
        std::string Title = "";
        u32 Width, Height = 0;

        EventCallbackFunction EmitEvent;
    };

    class Window : public EventEmitter
    {
    public:
        Window(const WindowSettings& settings);
        ~Window();

        void BeginFrame();
        void PollEvents();
        void EndFrame();

        GraphicsContext& GetContext() const { return *m_GraphicsContext; }
        GLFWwindow* GetWindowHandle() const { return m_Window; }
        u32 GetWidth() const { return m_WindowData.Width; }
        u32 GetHeight() const { return m_WindowData.Height; }

    public:
        static Ref<Window> Create(const WindowSettings& settings);
        static Window& GetMainWindow() { return *s_MainWindow; }

    private:
        static int s_WindowCount;
        static Ref<Window> s_MainWindow;

    private:
        Ref<GraphicsContext> m_GraphicsContext;
        WindowData m_WindowData;
        GLFWwindow* m_Window;
    };
}