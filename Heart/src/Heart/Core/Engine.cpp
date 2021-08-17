#include "htpch.h"
#include "Engine.h"

#include "Heart/Renderer/RenderApi.h"
#include "Heart/Renderer/Renderer.h"

namespace Heart
{
    Engine* Engine::s_Instance = nullptr;

    Engine::Engine()
    {
        HT_ENGINE_ASSERT(!s_Instance, "Engine instance already exists");
        s_Instance = this;

        Renderer::Initialize(RenderApi::Type::Vulkan);

        WindowSettings windowSettings = WindowSettings();
        m_Window = Window::Create(windowSettings);
        SubscribeToEmitter(&GetWindow());

        m_ImGuiInstance = ImGuiInstance::Create(GetWindow());

        HT_ENGINE_LOG_INFO("Engine initialized");
    }

    Engine::~Engine()
    {
        UnsubscribeFromEmitter(&GetWindow());
        Renderer::Shutdown();
    }

    void Engine::PushLayer(Layer* layer)
    {
        m_Layers.push_back(layer);
        layer->OnAttach();
    }

    void Engine::OnEvent(Event& event)
    {        
        event.Map<WindowResizeEvent>(HT_BIND_EVENT_FN(Engine::OnWindowResize));
        event.Map<WindowCloseEvent>(HT_BIND_EVENT_FN(Engine::OnWindowClose));
    }

    bool Engine::OnWindowResize(WindowResizeEvent& event)
    {
        HT_ENGINE_LOG_INFO("Window resized");

        return false;
    }

    bool Engine::OnWindowClose(WindowCloseEvent& event)
    {
        m_Running = false;
        return true;
    }

    void Engine::Run()
    {
        while (m_Running)
        {
            m_Window->OnUpdate();

            // Layer update
            for (auto layer : m_Layers)
                layer->OnUpdate();

            // ImGui render
            m_ImGuiInstance->BeginFrame();
            for (auto layer : m_Layers)
                layer->OnImGuiRender();
            m_ImGuiInstance->EndFrame();
        }
    }
}