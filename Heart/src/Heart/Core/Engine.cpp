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
        m_WindowSubscribeId = m_Window->Subscribe(HT_BIND_EVENT_FN(Engine::OnWindowEvent));

        //m_ImGuiInstance = new ImGuiInstance();

        HT_ENGINE_LOG_INFO("Engine initialized");
    }

    Engine::~Engine()
    {
        m_Window->Unsubscribe(m_WindowSubscribeId);
    }

    void Engine::PushLayer(Layer* layer)
    {
        m_Layers.push_back(layer);
        layer->OnAttach();
    }

    void Engine::OnWindowEvent(Event& event)
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