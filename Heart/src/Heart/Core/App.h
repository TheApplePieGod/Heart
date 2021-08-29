#pragma once

#include "Heart/Core/Layer.h"
#include "Heart/Core/Window.h"
#include "Heart/Events/Event.h"
#include "Heart/Events/WindowEvents.h"
#include "Heart/ImGui/ImGuiInstance.h"

extern int main(int argc, char** argv);

namespace Heart
{
    class App : public EventListener
    {
    public:
        App(const std::string& windowName = "Heart Engine");
        ~App();

        inline static App& Get() { return *s_Instance; }
        inline ImGuiInstance& GetImGuiInstance() { return m_ImGuiInstance; }
        inline Window& GetWindow() const { return *m_Window; }
        inline u64 GetFrameCount() const { return m_FrameCount; }
        inline void Close() { m_Running = false; };

        void PushLayer(Layer* layer);

    private:
        void Run();      
        void OnEvent(Event& event) override;
        bool OnWindowResize(WindowResizeEvent& event);
        bool OnWindowClose(WindowCloseEvent& event);

    private:
        std::vector<Layer*> m_Layers;
        ImGuiInstance m_ImGuiInstance;
        Ref<Window> m_Window;
        bool m_Running = true;
        bool m_Minimized = false;
        u64 m_FrameCount = 0;
        double m_LastFrameTime = 0.0;

    private:
        static App* s_Instance;
        friend int ::main(int argc, char** argv);
    };
}