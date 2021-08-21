#include "htpch.h"
#include "App.h"

#include "Heart/Renderer/RenderApi.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Renderer/SceneRenderer.h"

namespace Heart
{
    App* App::s_Instance = nullptr;

    App::App()
    {
        HE_ENGINE_ASSERT(!s_Instance, "App instance already exists");
        s_Instance = this;

        Renderer::Initialize(RenderApi::Type::Vulkan);

        WindowSettings windowSettings = WindowSettings();
        m_Window = Window::Create(windowSettings);
        SubscribeToEmitter(&GetWindow());

        m_ImGuiInstance.Initialize();

        HE_ENGINE_LOG_INFO("App initialized");
    }

    App::~App()
    {
        UnsubscribeFromEmitter(&GetWindow());

        m_ImGuiInstance.Shutdown();

        Renderer::Shutdown();
    }

    void App::PushLayer(Layer* layer)
    {
        m_Layers.push_back(layer);
        layer->OnAttach();
    }

    void App::OnEvent(Event& event)
    {        
        event.Map<WindowResizeEvent>(HE_BIND_EVENT_FN(App::OnWindowResize));
        event.Map<WindowCloseEvent>(HE_BIND_EVENT_FN(App::OnWindowClose));
    }

    bool App::OnWindowResize(WindowResizeEvent& event)
    {
        //HT_ENGINE_LOG_INFO("Window resized");
        if (event.GetWidth() == 0 || event.GetHeight() == 0)
        {
            m_Minimized = true;
            return false;
        }

        Renderer::OnWindowResize(m_Window->GetContext(), event.GetWidth(), event.GetHeight());
        m_Minimized = false;

        return false;
    }

    bool App::OnWindowClose(WindowCloseEvent& event)
    {
        m_Running = false;
        return true;
    }

    void App::Run()
    {
        while (m_Running)
        {
            m_Window->PollEvents();

            if (m_Minimized)
                continue;

            m_Window->BeginFrame();

            // Layer update
            for (auto layer : m_Layers)
                layer->OnUpdate();

            // ImGui render
            m_ImGuiInstance.BeginFrame();
            for (auto layer : m_Layers)
                layer->OnImGuiRender();
            m_ImGuiInstance.EndFrame();

            m_Window->EndFrame();
        }
    }
}