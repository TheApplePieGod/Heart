#include "htpch.h"
#include "Engine.h"

namespace Heart
{
    Engine* Engine::s_Instance = nullptr;

    Engine::Engine()
    {
        HT_ENGINE_ASSERT(!s_Instance, "Engine instance already exists");
        s_Instance = this;

        WindowSettings windowSettings = WindowSettings();
        m_Window = Window::Create(windowSettings);
        SubscribeToEmitter(&GetWindow());

        //m_ImGuiInstance = new ImGuiInstance();

        HT_ENGINE_LOG_INFO("Engine initialized");
    }

    Engine::~Engine()
    {
        UnsubscribeFromEmitter(&GetWindow());
    }

    void Engine::PushLayer(Layer* layer)
    {
        m_Layers.push_back(layer);
        layer->OnAttach();
    }

    void Engine::OnEvent(Event& event)
    {        
        event.Map<WindowResizeEvent>(HT_BIND_EVENT_FN(Engine::OnWindowResize));
    }

    bool Engine::OnWindowResize(WindowResizeEvent& event)
    {
        HT_ENGINE_LOG_INFO("Window resized");

        return false;
    }

    void Engine::Run()
    {
        while (m_Running)
        {
            m_Window->OnUpdate();
        }
    }
}