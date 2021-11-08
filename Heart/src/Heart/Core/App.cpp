#include "htpch.h"
#include "App.h"

#include "Heart/Core/Timing.h"

namespace Heart
{
    App* App::s_Instance = nullptr;

    App::App(const std::string& windowName)
    {
        HE_ENGINE_ASSERT(!s_Instance, "App instance already exists");
        s_Instance = this;

        Timer timer = Timer("App initialization");
        #ifdef HE_DEBUG
            HE_ENGINE_LOG_INFO("Running Heart in Debug mode");
        #else
            HE_ENGINE_LOG_INFO("Running Heart in Release mode");
        #endif

        WindowSettings windowSettings = WindowSettings(windowName);
        InitializeGraphicsApi(RenderApi::Type::OpenGL, windowSettings);

        HE_ENGINE_LOG_INFO("App initialized");
    }

    App::~App()
    {
        ShutdownGraphicsApi();

        HE_ENGINE_LOG_INFO("Shutdown complete");
    }

    void App::PushLayer(Layer* layer)
    {
        m_Layers.push_back(layer);
        layer->OnAttach();
    }

    void App::SwitchGraphicsApi(RenderApi::Type type)
    {
        m_SwitchingApi = type;
    }
    
    void App::InitializeGraphicsApi(RenderApi::Type type, const WindowSettings& windowSettings)
    {
        Renderer::Initialize(type);

        m_Window = Window::Create(windowSettings);
        SubscribeToEmitter(&GetWindow());
        Window::SetMainWindow(m_Window);

        m_ImGuiInstance.Initialize();
    }

    void App::ShutdownGraphicsApi()
    {
        for (auto layer : m_Layers)
            layer->OnDetach();

        UnsubscribeFromEmitter(&GetWindow());
        Window::SetMainWindow(nullptr);

        m_ImGuiInstance.Shutdown();

        Renderer::Shutdown();

        m_Window.reset();
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
        Close();
        return true;
    }

    void App::CheckForGraphicsApiSwitch()
    {
        if (m_SwitchingApi != RenderApi::Type::None)
        {
            WindowSettings windowSettings = {
                m_Window->GetTitle(),
                m_Window->GetWidth(),
                m_Window->GetHeight()
            };
            bool fullscreen = m_Window->IsFullscreen();

            ShutdownGraphicsApi();
            AggregateTimer::ClearTimeMap();

            InitializeGraphicsApi(m_SwitchingApi, windowSettings);
            m_Window->SetFullscreen(fullscreen);

            for (auto layer : m_Layers)
                layer->OnAttach();

            m_SwitchingApi = RenderApi::Type::None;
        }
    }

    void App::Run()
    {
        while (m_Running)
        {
            HE_PROFILE_FRAME();

            double currentFrameTime = m_Window->GetWindowTime();
            Timestep ts = Timestep(currentFrameTime - m_LastFrameTime);
            m_LastFrameTime = currentFrameTime;

            m_Window->PollEvents();

            if (m_Minimized)
                continue;

            m_Window->BeginFrame();

            // Layer update
            for (auto layer : m_Layers)
                layer->OnUpdate(ts);

            // ImGui render
            m_ImGuiInstance.BeginFrame();
            for (auto layer : m_Layers)
                layer->OnImGuiRender();
            m_ImGuiInstance.EndFrame();

            m_Window->EndFrame();
            m_FrameCount++;

            CheckForGraphicsApiSwitch();
            AggregateTimer::ResetAggregateTimes();
        }
    }
}