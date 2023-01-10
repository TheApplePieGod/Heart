#include "hepch.h"
#include "App.h"

#include "Heart/Core/Layer.h"
#include "Heart/Core/Window.h"
#include "Heart/Core/Timing.h"
#include "Heart/ImGui/ImGuiInstance.h"
#include "Heart/Physics/PhysicsWorld.h"
#include "Heart/Events/WindowEvents.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Task/JobManager.h"
#include "Heart/Util/PlatformUtils.h"
#include "Flourish/Api/Context.h"
#include "Flourish/Core/Log.h"

namespace Heart
{
    App::App(const HStringView8& windowName)
    {
        if (s_Instance) return;
        s_Instance = this;

        Heart::Logger::Initialize(windowName.Data());

        Timer timer = Timer("App initialization");
        #ifdef HE_DEBUG
            HE_ENGINE_LOG_INFO("Running Heart in Debug mode");
        #else
            HE_ENGINE_LOG_INFO("Running Heart in Release mode");
        #endif
        
        u32 taskThreads = std::thread::hardware_concurrency() - 2;
        HE_ENGINE_LOG_INFO("Using {0} task & job worker threads", taskThreads);
        JobManager::Initialize(taskThreads);
        TaskManager::Initialize(taskThreads);
        
        /*
        Task taskA = TaskManager::Schedule([]()
        {
            HE_ENGINE_LOG_WARN("Starting task A");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            HE_ENGINE_LOG_WARN("Task A finish");
        });
        
        Task taskB = TaskManager::Schedule([]()
        {
            HE_ENGINE_LOG_WARN("Starting task B");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            HE_ENGINE_LOG_WARN("Task B finish");
        }, taskA);
        
        Task taskC = TaskManager::Schedule([]()
        {
            HE_ENGINE_LOG_WARN("Starting task C");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            HE_ENGINE_LOG_WARN("Task C finish");
        }, taskA);
        
        TaskGroup group({ taskB, taskC });
        
        Task taskD = TaskManager::Schedule([]()
        {
            HE_ENGINE_LOG_WARN("Starting task D");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            HE_ENGINE_LOG_WARN("Task D finish");
        }, group);
         
        Task taskP = TaskManager::Schedule([]()
        {
            HE_ENGINE_LOG_WARN("Starting task P");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            HE_ENGINE_LOG_WARN("Task P finish");
        });
         
        HE_ENGINE_LOG_WARN("Waiting for tasks");
        taskD.Wait();
        HE_ENGINE_LOG_WARN("Done waiting");
        */
        
        /*
        Job job = JobManager::Schedule([](size_t index)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            HE_ENGINE_LOG_WARN("JOB {0}", index);
        }, 500);
        
        job.Wait();
        */
        
        WindowCreateInfo windowCreateInfo = WindowCreateInfo(windowName);
        InitializeGraphicsApi(windowCreateInfo);

        // Init services
        PhysicsWorld::Initialize();
        AssetManager::Initialize();
        ScriptingEngine::Initialize();

        HE_ENGINE_LOG_INFO("App initialized");
    }

    App::~App()
    {
        // Shutdown services
        ScriptingEngine::Shutdown();
        AssetManager::Shutdown();

        for (auto layer : m_Layers)
            layer->OnDetach();

        ShutdownGraphicsApi();
        
        TaskManager::Shutdown();
        JobManager::Shutdown();
        
        PlatformUtils::ShutdownPlatform();

        HE_ENGINE_LOG_INFO("Shutdown complete");
    }

    void App::PushLayer(const Ref<Layer>& layer)
    {
        m_Layers.Add(layer);
        layer->OnAttach();
    }

    void App::SwitchAssetsDirectory(const HStringView8& newDirectory)
    {
        m_SwitchingAssetsDirectory = newDirectory;
    }

    void App::InitializeGraphicsApi(const WindowCreateInfo& windowCreateInfo)
    {
        Flourish::Logger::SetLogFunction([](Flourish::LogLevel level, const char* message)
        {
            Logger::GetEngineLogger().log((spdlog::level::level_enum)level, message);
        });

        Flourish::ContextInitializeInfo initInfo;
        initInfo.ApplicationName = "Heart";
        initInfo.Backend = Flourish::BackendType::Vulkan;
        initInfo.FrameBufferCount = 2;
        initInfo.UseReversedZBuffer = true;
        initInfo.RequestedFeatures.IndependentBlend = true;
        initInfo.RequestedFeatures.WideLines = true;
        initInfo.RequestedFeatures.SamplerAnisotropy = true;
        Flourish::Context::Initialize(initInfo);

        m_Window = Window::Create(windowCreateInfo);
        SubscribeToEmitter(&GetWindow());
        Window::SetMainWindow(m_Window);

        m_ImGuiInstance = CreateRef<ImGuiInstance>(m_Window);
    }

    void App::ShutdownGraphicsApi()
    {
        UnsubscribeFromEmitter(&GetWindow());

        Flourish::Context::Shutdown([this]()
        {
            m_ImGuiInstance.reset();

            Window::SetMainWindow(nullptr);
            m_Window.reset();
        });
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

        // Renderer::OnWindowResize(m_Window->GetContext(), event.GetWidth(), event.GetHeight());
        m_Minimized = false;

        return false;
    }

    bool App::OnWindowClose(WindowCloseEvent& event)
    {
        Close();
        return true;
    }

    void App::CheckForAssetsDirectorySwitch()
    {
        if (!m_SwitchingAssetsDirectory.IsEmpty())
        {
            AssetManager::UpdateAssetsDirectory(m_SwitchingAssetsDirectory);

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

            auto timer = AggregateTimer("App::Run - PollEvents");
            m_Window->PollEvents();
            timer.Finish();

            if (m_Minimized)
                continue;

            // Begin frame
            timer = AggregateTimer("App::Run - Begin frame");
            Flourish::Context::BeginFrame();
            AssetManager::OnUpdate();
            m_Window->BeginFrame();
            timer.Finish();

            // Layer update
            for (auto layer : m_Layers)
                layer->OnUpdate(m_LastTimestep);

            // ImGui render
            m_ImGuiInstance->BeginFrame();
            for (auto layer : m_Layers)
                layer->OnImGuiRender();

            // End frame
            timer = AggregateTimer("App::Run - End frame");
            m_ImGuiInstance->EndFrame();
            m_Window->EndFrame();
            Flourish::Context::EndFrame();
            m_FrameCount++;
            timer.Finish();

            CheckForAssetsDirectorySwitch();
            AggregateTimer::EndFrame();
        }
    }
}
