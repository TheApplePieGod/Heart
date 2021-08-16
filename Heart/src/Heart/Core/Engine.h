#pragma once

#include "Heart/Core/Layer.h"
#include "Heart/Core/Window.h"
#include "Heart/Events/Event.h"
#include "Heart/Events/WindowEvents.h"
#include "Heart/ImGui/ImGuiInstance.h"

extern int main(int argc, char** argv);

namespace Heart
{
    class Engine : public EventListener
    {
    public:
        Engine();
        ~Engine();

        inline static Engine& Get() { return *s_Instance; }
        inline ImGuiInstance& GetImGuiInstance() const { return *m_ImGuiInstance; }
        inline Window& GetWindow() const { return *m_Window; }

        void PushLayer(Layer* layer);

    private:
        void Run();
        inline void Stop() { m_Running = false; };
        void OnEvent(Event& event) override;
        bool OnWindowResize(WindowResizeEvent& event);

    private:
        std::vector<Layer*> m_Layers;
        Scope<ImGuiInstance> m_ImGuiInstance;
        Scope<Window> m_Window;
        bool m_Running = true;

    private:
        static Engine* s_Instance;
        friend int ::main(int argc, char** argv);
    };
}