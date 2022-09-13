#include "hepch.h"
#include "App.h"

#include "Heart/Core/Layer.h"
#include "Heart/Core/Window.h"
#include "Heart/Core/Timing.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/ImGui/ImGuiInstance.h"
#include "Heart/Events/WindowEvents.h"
#include "Heart/Events/AppEvents.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Util/PlatformUtils.h"

namespace Heart
{
    App::App(const HStringView8& windowName)
    {
        HE_ENGINE_ASSERT(!s_Instance, "App instance already exists");
        s_Instance = this;

        PlatformUtils::InitializePlatform();

        Timer timer = Timer("App initialization");
        #ifdef HE_DEBUG
            HE_ENGINE_LOG_INFO("Running Heart in Debug mode");
        #else
            HE_ENGINE_LOG_INFO("Running Heart in Release mode");
        #endif

        WindowCreateInfo windowCreateInfo = WindowCreateInfo(windowName);
        InitializeGraphicsApi(RenderApi::Type::Vulkan, windowCreateInfo);

        // Init services
        AssetManager::Initialize();
        ScriptingEngine::Initialize();

        HE_ENGINE_LOG_INFO("App initialized");
    }

    App::~App()
    {
        // Shutdown services
        ScriptingEngine::Shutdown();
        AssetManager::Shutdown();

        ShutdownGraphicsApi();

        PlatformUtils::ShutdownPlatform();

        HE_ENGINE_LOG_INFO("Shutdown complete");
    }

    void App::PushLayer(const Ref<Layer>& layer)
    {
        m_Layers.Add(layer);
        layer->OnAttach();
    }

    void App::SwitchGraphicsApi(RenderApi::Type type)
    {
        m_SwitchingApi = type;
    }
    
    void App::SwitchAssetsDirectory(const HStringView8& newDirectory)
    {
        m_SwitchingAssetsDirectory = newDirectory;
    }

    void App::InitializeGraphicsApi(RenderApi::Type type, const WindowCreateInfo& windowCreateInfo)
    {
        Renderer::Initialize(type);

        m_Window = Window::Create(windowCreateInfo);
        SubscribeToEmitter(&GetWindow());
        Window::SetMainWindow(m_Window);

        m_ImGuiInstance = CreateRef<ImGuiInstance>(m_Window);

        AppGraphicsInitEvent event;
        Emit(event);
    }

    void App::ShutdownGraphicsApi()
    {
        for (auto layer : m_Layers)
            layer->OnDetach();

        UnsubscribeFromEmitter(&GetWindow());

        AppGraphicsShutdownEvent event;
        Emit(event);

        m_ImGuiInstance.reset();

        Renderer::Shutdown();

        Window::SetMainWindow(nullptr);
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
            WindowCreateInfo windowCreateInfo = {
                m_Window->GetTitle(),
                m_Window->GetWidth(),
                m_Window->GetHeight()
            };
            bool fullscreen = m_Window->IsFullscreen();

            AssetManager::UnloadAllAssets();

            ShutdownGraphicsApi();
            AggregateTimer::ClearTimeMap();

            InitializeGraphicsApi(m_SwitchingApi, windowCreateInfo);
            m_Window->SetFullscreen(fullscreen);

            for (auto layer : m_Layers)
                layer->OnAttach();

            m_SwitchingApi = RenderApi::Type::None;
        }
    }

    void App::CheckForAssetsDirectorySwitch()
    {
        if (!m_SwitchingAssetsDirectory.IsEmpty())
        {
            AssetManager::UpdateAssetsDirectory(m_SwitchingAssetsDirectory);
            
            // TEMPORARY SOLUTION
            // Until we have dedicated projects and I figure out what exactly needs to happen when
            // the assets directory changes, force a full reload on the graphics backend
            m_SwitchingApi = Renderer::GetApiType();

            m_SwitchingAssetsDirectory.Clear();
        }
    }

    void App::Close()
    {
        m_Running = false;
    }

    void App::CloseWithConfirmation()
    {
        #ifdef HE_PLATFORM_WINDOWS
            int selection = MessageBox(HWND_DESKTOP, "Are you sure you want to quit? You may have unsaved changes.", "Confirmation", MB_OKCANCEL | WS_POPUP);
            if (selection != 1)
               return;
        #endif

        m_Running = false;
    }

    void App::Run()
    {
        while (m_Running)
        {
            HE_PROFILE_FRAME();

            double currentFrameTime = m_Window->GetWindowTime();
            m_LastTimestep = Timestep(currentFrameTime - m_LastFrameTime);
            m_LastFrameTime = currentFrameTime;

            m_Window->PollEvents();

            if (m_Minimized)
                continue;

            AssetManager::OnUpdate();

            m_Window->BeginFrame();

            // Layer update
            for (auto layer : m_Layers)
                layer->OnUpdate(m_LastTimestep);

            // ImGui render
            m_ImGuiInstance->BeginFrame();
            for (auto layer : m_Layers)
                layer->OnImGuiRender();
            m_ImGuiInstance->EndFrame();

            m_Window->EndFrame();
            m_FrameCount++;

            CheckForAssetsDirectorySwitch();
            CheckForGraphicsApiSwitch();
            AggregateTimer::EndFrame();
        }
    }
}