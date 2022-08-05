#pragma once

#include "Heart/Events/EventEmitter.h"
#include "Heart/Container/HString.h"

class GLFWwindow;
namespace Heart
{
    /*! @brief Window creation settings. */
    struct WindowCreateInfo
    {
        HString Title;
        u32 Width, Height;

        WindowCreateInfo(const HStringView& title = "Window",
			        u32 width = 1920,
			        u32 height = 1080)
			: Title(title), Width(width), Height(height)
		{}
    };

    // TODO: describe method lifecycles
    class GraphicsContext;
    class Window : public EventEmitter
    {
    public:
        /**
         * @brief Default constructor.
         *
         * @param createInfo The creation settings for this window.
         */
        Window(const WindowCreateInfo& createInfo);

        /*! @brief Default destructor. */
        ~Window();

        /*! @brief Begin the window's frame. */
        void BeginFrame();

        /*! @brief Poll the window for various input/misc events. */
        void PollEvents();

        /*! @brief End the window's frame. */
        void EndFrame();

        /*! @brief Disable the cursor while this window is focused. */
        void DisableCursor();

        /*! @brief Enable the cursor while this window is focused. */
        void EnableCursor();

        /*! @brief Updates the window's title. */
        void SetWindowTitle(const char* newTitle);

        /**
         * @brief Set the fullscreen status of the window.
         * 
         * @param fullscreen True if the window should be fullscreen and false otherwise.
         */
        void SetFullscreen(bool fullscreen);

        /*! @brief Toggle the current fullscreen status of the window. */
        void ToggleFullscreen();

        /*! @brief Get the current fullscreen status of the window. */
        bool IsFullscreen();

        /*! @brief Get the window's graphics context. */
        inline GraphicsContext& GetContext() const { return *m_GraphicsContext; }

        /*! @brief Get the window's underlying GLFW handle. */
        inline GLFWwindow* GetWindowHandle() const { return m_Window; }

        /*! @brief Get the current width of the window. */
        inline u32 GetWidth() const { return m_WindowData.Width; }

        /*! @brief Get the current height of the window. */
        inline u32 GetHeight() const { return m_WindowData.Height; }

        /*! @brief Get the window's current title. */
        inline HString GetTitle() const { return m_WindowData.Title; }

        /*! @brief Get the window's elapsed lifetime in milliseconds. */
        double GetWindowTime();

    public:
        /**
         * @brief Statically create a new window object.
         * 
         * @param createInfo The creation settings for the window.
         * @return A ref to the new window object.
         */
        static Ref<Window> Create(const WindowCreateInfo& createInfo);

        /*! @brief Get the global main window object. */
        static Window& GetMainWindow() { return *s_MainWindow; }

        /**
         * @brief Set the global main window object.
         * 
         * @param newWindow A ref to the window object to set as the main window.
         */
        inline static void SetMainWindow(Ref<Window> newWindow)
        {
            HE_ENGINE_LOG_TRACE("Main window object set to {0}", static_cast<void*>(newWindow.get()));
            s_MainWindow = newWindow;
        }

    private:
        struct WindowData
        {
            HString Title = "";
            u32 Width, Height = 0;

            EventCallbackFunction EmitEvent;
        };

    private:
        inline static int s_WindowCount = 0;
        inline static Ref<Window> s_MainWindow = nullptr;

    private:
        Ref<GraphicsContext> m_GraphicsContext;
        int m_SavedWindowSizeAndPosition[4]; // used when toggling fullscreen
        WindowData m_WindowData;
        GLFWwindow* m_Window;

        friend class App;
    };
}