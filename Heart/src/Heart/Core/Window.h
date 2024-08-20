#pragma once

#include "Heart/Events/EventEmitter.h"
#include "Heart/Container/HString8.h"

namespace Flourish
{
    class RenderContext;
    class CommandBuffer;
}

namespace Heart
{
    /*! @brief Window creation settings. */
    struct WindowCreateInfo
    {
        HString8 Title;
        u32 Width, Height;

        WindowCreateInfo(const HStringView8& title = "Window",
			        u32 width = 1920,
			        u32 height = 1080)
			: Title(title), Width(width), Height(height)
		{}
    };

    // TODO: describe method lifecycles
    class WindowResizeEvent;
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
        virtual ~Window();

        /** 
         * @brief Poll the window for various input/misc events.
         * @return true if the window was recreated for any reason
         */
        virtual bool PollEvents() = 0;

        /*! @brief End the window's frame. */
        virtual void EndFrame() = 0;

        /*! @brief Disable the cursor while this window is focused. */
        virtual void DisableCursor() = 0;

        /*! @brief Enable the cursor while this window is focused. */
        virtual void EnableCursor() = 0;

        /*! @brief Updates the window's title. */
        virtual void SetWindowTitle(const char* newTitle) = 0;

        /**
         * @brief Set the fullscreen status of the window.
         * 
         * @param fullscreen True if the window should be fullscreen and false otherwise.
         */
        virtual void SetFullscreen(bool fullscreen) = 0;

        /*! @brief Toggle the current fullscreen status of the window. */
        virtual void ToggleFullscreen() = 0;

        /*! @brief Get the current fullscreen status of the window. */
        virtual bool IsFullscreen() const = 0;

        /*! @brief Get the window's graphics context. */
        inline Flourish::RenderContext* GetRenderContext() const { return m_RenderContext.get(); }

        /*! @brief Get the window's underlying handle. */
        inline void* GetWindowHandle() const { return m_Window; }

        /*! @brief Get the current width of the window. */
        inline u32 GetWidth() const { return m_WindowData.Width; }

        /*! @brief Get the current height of the window. */
        inline u32 GetHeight() const { return m_WindowData.Height; }

        /*! @brief Get the window's current title. */
        inline HString8 GetTitle() const { return m_WindowData.Title; }

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

    protected:
        struct WindowData
        {
            HString8 Title = "";
            u32 Width, Height = 0;

            EventCallbackFunction EmitEvent;
        };

    protected:
        inline static int s_WindowCount = 0;
        inline static Ref<Window> s_MainWindow = nullptr;

    protected:
        Ref<Flourish::RenderContext> m_RenderContext;
        int m_SavedWindowSizeAndPosition[4]; // used when toggling fullscreen
        WindowData m_WindowData;
        void* m_Window;

        friend class App;

    private:
        void EmitEvent(Event& event);
        bool OnWindowResize(WindowResizeEvent& event);
    };
}
