#pragma once

#include "Heart/Events/EventEmitter.h"

class GLFWwindow;
namespace Heart
{
    struct WindowSettings
    {
        std::string Title;
        u32 Width, Height;

        WindowSettings(const std::string& title = "Window",
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

    class GraphicsContext;
    class Window : public EventEmitter
    {
    public:
        Window(const WindowSettings& settings);
        ~Window();

        void BeginFrame();
        void PollEvents();
        void EndFrame();

        void DisableCursor();
        void EnableCursor();
        void SetFullscreen(bool fullscreen);
        void ToggleFullscreen();
        bool IsFullscreen();

        inline GraphicsContext& GetContext() const { return *m_GraphicsContext; }
        inline GLFWwindow* GetWindowHandle() const { return m_Window; }
        inline u32 GetWidth() const { return m_WindowData.Width; }
        inline u32 GetHeight() const { return m_WindowData.Height; }
        inline std::string GetTitle() const { return m_WindowData.Title; }
        double GetWindowTime();

    public:
        static Ref<Window> Create(const WindowSettings& settings);
        static Window& GetMainWindow() { return *s_MainWindow; }
        inline static void SetMainWindow(Ref<Window> newWindow) { HE_ENGINE_LOG_TRACE("Main window object set to {0}", static_cast<void*>(newWindow.get())); s_MainWindow = newWindow; }

    private:
        static int s_WindowCount;
        static Ref<Window> s_MainWindow;

    private:
        Ref<GraphicsContext> m_GraphicsContext;
        int m_SavedWindowSizeAndPosition[4]; // used when toggling fullscreen
        WindowData m_WindowData;
        GLFWwindow* m_Window;

        friend class App;
    };
}