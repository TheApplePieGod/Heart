#include "htpch.h"
#include "Engine.h"

#include "Heart/Renderer/RenderApi.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Renderer/SceneRenderer.h"

namespace Heart
{
    Engine* Engine::s_Instance = nullptr;

    Engine::Engine()
    {
        HE_ENGINE_ASSERT(!s_Instance, "Engine instance already exists");
        s_Instance = this;

        Renderer::Initialize(RenderApi::Type::Vulkan);

        WindowSettings windowSettings = WindowSettings();
        m_Window = Window::Create(windowSettings);
        SubscribeToEmitter(&GetWindow());

        SceneRenderer::Initialize();

        m_ImGuiInstance.Initialize();

        HE_ENGINE_LOG_INFO("Engine initialized");
    }

    Engine::~Engine()
    {
        UnsubscribeFromEmitter(&GetWindow());

        m_ImGuiInstance.Shutdown();

        SceneRenderer::Shutdown();

        Renderer::Shutdown();
    }

    void Engine::PushLayer(Layer* layer)
    {
        m_Layers.push_back(layer);
        layer->OnAttach();
    }

    void Engine::OnEvent(Event& event)
    {        
        event.Map<WindowResizeEvent>(HE_BIND_EVENT_FN(Engine::OnWindowResize));
        event.Map<WindowCloseEvent>(HE_BIND_EVENT_FN(Engine::OnWindowClose));
    }

    bool Engine::OnWindowResize(WindowResizeEvent& event)
    {
        //HT_ENGINE_LOG_INFO("Window resized");
        if (event.GetWidth() == 0 || event.GetHeight() == 0)
        {
            // TODO: minimized
            return false;
        }

        Renderer::OnWindowResize(m_Window->GetContext(), event.GetWidth(), event.GetHeight());

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
            m_Window->BeginFrame();

            // Layer update
            for (auto layer : m_Layers)
                layer->OnUpdate();

            SceneRenderer::Bind();

            // ImGui render
            m_ImGuiInstance.BeginFrame();
            for (auto layer : m_Layers)
                layer->OnImGuiRender();
            m_ImGuiInstance.EndFrame();

            SceneRenderer::Render(m_Window->GetContext());

            m_Window->EndFrame();
        }
    }
}