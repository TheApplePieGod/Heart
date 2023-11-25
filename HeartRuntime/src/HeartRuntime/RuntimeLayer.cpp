#include "hepch.h"
#include "RuntimeLayer.h"

#include "HeartRuntime/RuntimeApp.h"
#include "Heart/Core/Window.h"
#include "Heart/ImGui/ImGuiInstance.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Util/FilesystemUtils.h"
#include "nlohmann/json.hpp"

#ifdef HE_PLATFORM_ANDROID
#include "Heart/Platform/Android/AndroidApp.h"
#endif

namespace HeartRuntime
{
    RuntimeLayer::RuntimeLayer(const std::filesystem::path& projectPath)
        : m_ProjectPath(projectPath)
    {
        m_RenderSettings.DrawGrid = false;
        m_RenderSettings.AsyncAssetLoading = true;
        m_RenderSettings.CopyEntityIdsTextureToCPU = false;
    }

    RuntimeLayer::~RuntimeLayer()
    {

    }

    void RuntimeLayer::OnAttach()
    {
        SubscribeToEmitter(&RuntimeApp::Get().GetWindow());

        LoadProject();

        HE_LOG_INFO("Runtime attached");
    }

    void RuntimeLayer::OnDetach()
    {
        UnsubscribeFromEmitter(&RuntimeApp::Get().GetWindow());

        m_Viewport.Shutdown();
        m_RuntimeScene.reset();
        m_RenderScene.Cleanup();

        HE_LOG_INFO("Runtime detached");
    }

    void RuntimeLayer::OnUpdate(Heart::Timestep ts)
    {
        m_SceneUpdateTask.Wait();

        m_RenderScene.CopyFromScene(m_RuntimeScene.get());
        
        m_SceneUpdateTask = Heart::TaskManager::Schedule([this, ts]()
        {
            m_RuntimeScene->OnUpdateRuntime(ts);
        }, Heart::Task::Priority::High, "SceneUpdate task");

        m_Viewport.OnImGuiRender(&m_RenderScene, m_RuntimeScene.get(), m_RenderSettings);
        m_DevPanel.OnImGuiRender(m_RuntimeScene.get(), m_RenderSettings);
    }

    void RuntimeLayer::OnEvent(Heart::Event& event)
    {
        event.Map<Heart::KeyPressedEvent>(HE_BIND_EVENT_FN(RuntimeLayer::KeyPressedEvent));
    }

    bool RuntimeLayer::KeyPressedEvent(Heart::KeyPressedEvent& event)
    {
        if (event.GetKeyCode() == Heart::KeyCode::F11)
            RuntimeApp::Get().GetWindow().ToggleFullscreen();
        if (event.GetKeyCode() == Heart::KeyCode::F12)
        {
            if (m_DevPanel.IsOpen())
            {
                RuntimeApp::Get().GetWindow().DisableCursor();
                m_DevPanel.SetOpen(false);
            }
            else
            {
                RuntimeApp::Get().GetWindow().EnableCursor();
                m_DevPanel.SetOpen(true);
            }
        }
        
        return true;
    }

    void RuntimeLayer::LoadProject()
    {
        HE_LOG_TRACE("Loading project '{0}'", m_ProjectPath.generic_u8string());

        u32 fileLength;
        unsigned char* data = Heart::FilesystemUtils::ReadFile(m_ProjectPath.generic_u8string(), fileLength);
        if (!data)
        {
            HE_LOG_ERROR("Unable to load project");
            throw std::exception();
        }

        #ifdef HE_PLATFORM_ANDROID
            auto assemblyPath = std::filesystem::path(Heart::AndroidApp::App->activity->internalDataPath)
                .append("ClientScripts.dll");
        #else
            auto assemblyPath = std::filesystem::path("scripting/ClientScripts.dll");
        #endif

        if (!std::filesystem::exists(assemblyPath))
        {
            HE_LOG_ERROR("Project assembly not found @ {0}", assemblyPath.generic_u8string().c_str());
            throw std::exception();
        }
        
        Heart::ScriptingEngine::LoadClientPlugin(assemblyPath.generic_u8string());

        RuntimeApp::Get().GetImGuiInstance().OverrideImGuiConfig(Heart::AssetManager::GetAssetsDirectory());
        RuntimeApp::Get().GetImGuiInstance().ReloadImGuiConfig();

        // TODO: eventually switch from loadedScene to default scene or something like that
        auto j = nlohmann::json::parse(data);
        if (!j.contains("loadedScene") || j["loadedScene"].empty())
        {
            HE_LOG_ERROR("Unable to load scene");
            throw std::exception();
        }

        Heart::UUID sceneAssetId = Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Scene, j["loadedScene"]);
        m_RuntimeScene = Heart::AssetManager::RetrieveAsset(sceneAssetId)
            ->EnsureValid<Heart::SceneAsset>()
            ->GetScene()
            ->Clone();
        m_RuntimeScene->StartRuntime();

        RuntimeApp::Get().GetWindow().DisableCursor();
    }
}
