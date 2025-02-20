#include "hepch.h"
#include "App.h"

#include "Heart/Core/Layer.h"
#include "Heart/Core/Window.h"
#include "Heart/Core/Timing.h"
#include "Heart/Input/Input.h"
#include "Heart/ImGui/ImGuiInstance.h"
#include "Heart/Physics/PhysicsWorld.h"
#include "Heart/Events/WindowEvents.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Renderer/Material.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Task/JobManager.h"
#include "Heart/Util/PlatformUtils.h"
#include "Heart/Util/FilesystemUtils.h"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/RenderContext.h"
#include "Flourish/Core/Log.h"

namespace Heart
{
    App::App()
    {
        if (s_Instance) return;
        s_Instance = this;

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

        // Run on main thread, since some platforms have issues otherwise
        if (!ScriptingEngine::Initialize())
        {
            // Show a different message in dist bc dotnet doesn't actually need to be installed
            #ifdef HE_DIST
                PlatformUtils::ShowMessageBox(
                    ".NET Initialization Failed!",
                    "Unable to initialize the scripting backend",
                    "error"
                );
            #else
                PlatformUtils::ShowMessageBox(
                    ".NET Initialization Failed!",
                    "Unable to start the .NET SDK.\n\nPlease ensure it is installed on your system.\n\nhttps://dotnet.microsoft.com/en-us/download/dotnet/8.0",
                    "error"
                );
            #endif

            throw std::exception();
        }
        
        // TODO: put in a task?
        InitializeGraphicsApi();
        HE_ENGINE_LOG_DEBUG("Graphics ready");

        // Init services
        TaskGroup initServices;
        initServices.AddTask(TaskManager::Schedule(
            [](){ PhysicsWorld::Initialize(); },
            Task::Priority::High, "PhysicsWorld Init")
        );
        initServices.AddTask(TaskManager::Schedule(
            [](){ AssetManager::Initialize(); },
            Task::Priority::High, "AssetManager Init")
        );
        
        initServices.Wait();

        HE_ENGINE_LOG_INFO("App initialized");
    }

    App::~App()
    {
        // Ensure all jobs are completed
        TaskManager::Shutdown();
        JobManager::Shutdown();
        
        // Shutdown services
        ScriptingEngine::Shutdown();
        AssetManager::Shutdown();

        for (auto layer : m_Layers)
            layer->OnDetach();

        ShutdownGraphicsApi();
        
        PlatformUtils::ShutdownPlatform();

        HE_ENGINE_LOG_INFO("Shutdown complete");
    }

    void App::OpenWindow(const WindowCreateInfo& windowInfo)
    {
        m_Window = Window::Create(windowInfo);
        SubscribeToEmitter(&GetWindow());
        Window::SetMainWindow(m_Window);

        if (m_ImGuiInstance)
            m_ImGuiInstance->UpdateWindow(m_Window);
        else
            m_ImGuiInstance = CreateRef<ImGuiInstance>(m_Window);
    }

    void App::PushLayer(const Ref<Layer>& layer)
    {
        m_Layers.Add(layer);
        layer->OnAttach();
    }

    void App::PopLayer()
    {
        if (m_Layers.IsEmpty()) return;

        m_Layers.Back()->OnDetach();
        m_Layers.Pop();
    }

    void App::SwitchAssetsDirectory(const HString8& newDirectory)
    {
        m_SwitchingAssetsDirectory = newDirectory;
    }

    void App::InitializeGraphicsApi()
    {
        Flourish::Logger::SetLogFunction([](Flourish::LogLevel level, const char* message)
        {
            Logger::GetEngineLogger().log((spdlog::level::level_enum)level, message);
        });

        Flourish::ContextInitializeInfo initInfo;
        initInfo.ApplicationName = "Heart";
        initInfo.Backend = Flourish::BackendType::Vulkan;
        initInfo.FrameBufferCount = 3;
        initInfo.UseReversedZBuffer = true;
        initInfo.RequestedFeatures.IndependentBlend = true;
        initInfo.RequestedFeatures.SamplerAnisotropy = true;
        initInfo.RequestedFeatures.RayTracing = true;
        initInfo.RequestedFeatures.PartiallyBoundResourceSets = true;
        initInfo.ReadFile = FilesystemUtils::ReadFile;
        Flourish::Context::Initialize(initInfo);
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
        int selection = PlatformUtils::ShowMessageBoxCancel(
            "Are you sure?",
            "Are you sure you want to quit? You may have unsaved changes.",
            "warning"
        );

        if (selection != 1)
            return;

        m_Running = false;
    }

    void App::Run()
    {
        while (m_Running)
        {
            HE_PROFILE_FRAME();

            auto currentFrameTime = std::chrono::steady_clock::now();
            auto step = std::chrono::duration_cast<std::chrono::milliseconds>(currentFrameTime - m_LastFrameTime).count();
            m_LastTimestep = Timestep(step);
            m_TimestepSamples[m_FrameCount % 5] = m_LastTimestep.StepMilliseconds();
            double averaged = 0.0;
            for (auto sample : m_TimestepSamples)
                averaged += sample;
            m_AveragedTimestep = averaged / m_TimestepSamples.size();
            m_LastFrameTime = currentFrameTime;

            auto timer = AggregateTimer("App::Run - PollEvents");
            if (m_Window->PollEvents())
            {
                // True here means the window was internally recreated, so we need to update objects
                // that depend on window internals

                m_ImGuiInstance->Recreate();
            }
            timer.Finish();

            if (!m_Minimized && m_Window->GetRenderContext()->Validate())
            {
                // Begin frame
                timer = AggregateTimer("App::Run - Begin frame");
                AssetManager::UnloadOldAssets();
                Flourish::Context::BeginFrame();
                m_ImGuiInstance->BeginFrame();
                timer.Finish();

                // Layer update
                timer = AggregateTimer("App::Run - Layer update");
                for (auto layer : m_Layers)
                    layer->OnUpdate(m_LastTimestep);
                timer.Finish();

                // End frame
                timer = AggregateTimer("App::Run - End frame");
                m_ImGuiInstance->EndFrame();
                m_Window->EndFrame();
                Input::EndFrame();
                Flourish::Context::EndFrame();
                m_FrameCount++;
                timer.Finish();

                CheckForAssetsDirectorySwitch();
            }

            AggregateTimer::EndFrame();
        }
    }
}
