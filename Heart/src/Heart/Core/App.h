#pragma once

#include "Heart/Core/Timestep.h"
#include "Heart/Events/EventEmitter.h"
#include "Heart/Renderer/Renderer.h"

extern int main(int argc, char** argv);

namespace Heart
{
    class Layer;
    class ImGuiInstance;
    class Window;
    struct WindowSettings;
    class WindowResizeEvent;
    class WindowCloseEvent;
    class App : public EventListener, public EventEmitter
    {
    public:
        App(const std::string& windowName = "Heart Engine");
        ~App();

        void PushLayer(const Ref<Layer>& layer);
        void SwitchGraphicsApi(RenderApi::Type type);
        void SwitchAssetsDirectory(const std::string& newDirectory);

        virtual void Close();
        void CloseWithConfirmation();

        inline static App& Get() { return *s_Instance; }
        inline ImGuiInstance& GetImGuiInstance() { return *m_ImGuiInstance; }
        inline Window& GetWindow() const { return *m_Window; }
        inline u64 GetFrameCount() const { return m_FrameCount; }
        inline Timestep GetLastTimestep() const { return m_LastTimestep; }

    protected:
        std::vector<Ref<Layer>> m_Layers;
        Ref<ImGuiInstance> m_ImGuiInstance;
        Ref<Window> m_Window;
        bool m_Running = true;
        bool m_Minimized = false;
        u64 m_FrameCount = 0;
        double m_LastFrameTime = 0.0;
        Timestep m_LastTimestep;

    private:
        void Run();
        void InitializeGraphicsApi(RenderApi::Type type, const WindowSettings& windowSettings);
        void ShutdownGraphicsApi();
        void CheckForGraphicsApiSwitch();
        void CheckForAssetsDirectorySwitch();
        void OnEvent(Event& event) override;
        bool OnWindowResize(WindowResizeEvent& event);
        bool OnWindowClose(WindowCloseEvent& event);

    private:
        RenderApi::Type m_SwitchingApi = RenderApi::Type::None;
        std::string m_SwitchingAssetsDirectory = "";

    private:
        static App* s_Instance;
        friend int ::main(int argc, char** argv);
    };
}