#pragma once

#include "Heart/Core/Timestep.h"
#include "Heart/Events/EventEmitter.h"
#include "Heart/Container/HString8.h"
#include "Heart/Container/HVector.hpp"

namespace Heart
{
    class Layer;
    class ImGuiInstance;
    class Window;
    struct WindowCreateInfo;
    class WindowResizeEvent;
    class WindowCloseEvent;
    class App : public EventListener, public EventEmitter
    {
    public:
        /**
         * @brief Default constructor.
         * 
         * @param windowName The initial name of the window.
         */
        App();

        /*! @brief Default destructor. */
        ~App();

        void Run();

        void OpenWindow(const WindowCreateInfo& windowInfo);

        /**
         * @brief Append a new layer to the top of the layer stack.
         * 
         * @param layer The layer to push. 
         */
        void PushLayer(const Ref<Layer>& layer);

        void PopLayer();
        
        /**
         * @brief Switch the root assets directory that is currently being used by the engine.
         * 
         * This can be called whenever during the frame because the switch happens
         * at the end of the frame. This is a destructive action that removes registered assets
         * and rescans in the new directory. 
         * 
         * @param newDirectory The absolute path of the new assets directory. 
         */
        void SwitchAssetsDirectory(const HString8& newDirectory);

        /**
         * @brief Stop running and close the application.
         * 
         * The close will occur after the frame is complete. This function can be
         * overridden to perform custom behavior when the application is signaled
         * to close. Ideally, App::Close() should be called in overridden functions.
         */
        virtual void Close();
        
        /**
         * @brief Close the app with a native confirmation dialog.
         * 
         * This function works identically to Close(), but it does NOT call it.
         * Additionally, this function is not called by default but instead can be used
         * in an overridden implementation of Close(). 
         */
        void CloseWithConfirmation();

        /*! @brief Get the static global instance of the application object. */
        inline static App& Get() { return *s_Instance; }

        /*! @brief Get the ImGui instance object associated with this application. */
        inline ImGuiInstance& GetImGuiInstance() { return *m_ImGuiInstance; }

        /*! @brief Get the main window associated with this application. */
        inline Window& GetWindow() const { return *m_Window; }

        /*! @brief Get the elapsed frame count of this application. */
        inline u64 GetFrameCount() const { return m_FrameCount; }

        /*! @brief Get the timestamp for the last frame */
        inline Timestep GetLastTimestep() const { return m_LastTimestep; }

        /*! @brief Get the average timestamp from the last 5 frames */
        inline Timestep GetAveragedTimestep() const { return m_AveragedTimestep; }

    protected:
        HVector<Ref<Layer>> m_Layers;
        Ref<ImGuiInstance> m_ImGuiInstance;
        Ref<Window> m_Window;
        bool m_Running = true;
        bool m_Minimized = false;
        u64 m_FrameCount = 1;
        std::chrono::time_point<std::chrono::steady_clock> m_LastFrameTime;
        std::array<double, 5> m_TimestepSamples;
        Timestep m_AveragedTimestep;
        Timestep m_LastTimestep;

    private:
        void InitializeGraphicsApi();
        void ShutdownGraphicsApi();
        void CheckForAssetsDirectorySwitch();
        void OnEvent(Event& event) override;
        bool OnWindowResize(WindowResizeEvent& event);
        bool OnWindowClose(WindowCloseEvent& event);

    private:
        HString8 m_SwitchingAssetsDirectory = "";

    private:
        inline static App* s_Instance = nullptr;
    };
}
