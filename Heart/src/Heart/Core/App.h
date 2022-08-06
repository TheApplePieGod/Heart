#pragma once

#include "Heart/Core/Timestep.h"
#include "Heart/Events/EventEmitter.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Container/HString8.h"
#include "Heart/Container/HVector.hpp"

extern int main(int argc, char** argv);

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
        App(const HStringView8& windowName = "Heart Engine");

        /*! @brief Default destructor. */
        ~App();

        /**
         * @brief Append a new layer to the top of the layer stack.
         * 
         * @param layer The layer to push. 
         */
        void PushLayer(const Ref<Layer>& layer);
        
        /**
         * @brief Switch the graphics api that is currently being used by the engine. 
         *
         * This can be called whenever during the frame because the switch happens
         * at the end of the frame.
         *  
         * @param type The new api type to switch to.
         */
        void SwitchGraphicsApi(RenderApi::Type type);
        
        /**
         * @brief Switch the root assets directory that is currently being used by the engine.
         * 
         * This can be called whenever during the frame because the switch happens
         * at the end of the frame. This is a destructive action that removes registered assets
         * and rescans in the new directory. 
         * 
         * @param newDirectory The absolute path of the new assets directory. 
         */
        void SwitchAssetsDirectory(const HStringView8& newDirectory);

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

    protected:
        HVector<Ref<Layer>> m_Layers;
        Ref<ImGuiInstance> m_ImGuiInstance;
        Ref<Window> m_Window;
        bool m_Running = true;
        bool m_Minimized = false;
        u64 m_FrameCount = 0;
        double m_LastFrameTime = 0.0;
        Timestep m_LastTimestep;

    private:
        void Run();
        void InitializeGraphicsApi(RenderApi::Type type, const WindowCreateInfo& windowCreateInfo);
        void ShutdownGraphicsApi();
        void CheckForGraphicsApiSwitch();
        void CheckForAssetsDirectorySwitch();
        void OnEvent(Event& event) override;
        bool OnWindowResize(WindowResizeEvent& event);
        bool OnWindowClose(WindowCloseEvent& event);

    private:
        RenderApi::Type m_SwitchingApi = RenderApi::Type::None;
        HString8 m_SwitchingAssetsDirectory = "";

    private:
        inline static App* s_Instance = nullptr;
        friend int ::main(int argc, char** argv);
    };
}