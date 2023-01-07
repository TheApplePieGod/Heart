#include "hepch.h"
#include "RuntimeLayer.h"

#include "HeartRuntime/RuntimeApp.h"
#include "Heart/Core/Window.h"
#include "Heart/ImGui/ImGuiInstance.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Util/FilesystemUtils.h"
#include "nlohmann/json.hpp"

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

        HE_LOG_INFO("Runtime detached");
    }

    void RuntimeLayer::OnUpdate(Heart::Timestep ts)
    {
        m_RuntimeScene->OnUpdateRuntime(ts);
    }

    void RuntimeLayer::OnImGuiRender()
    {
        m_Viewport.OnImGuiRender(m_RuntimeScene.get(), m_RenderSettings);
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
        u32 fileLength;
        unsigned char* data = Heart::FilesystemUtils::ReadFile(m_ProjectPath.generic_u8string(), fileLength);
        if (!data)
        {
            HE_LOG_ERROR("Unable to load project");
            throw std::exception();
        }

        auto assemblyPath = std::filesystem::path("project")
            .append("bin")
            .append("ClientScripts.dll");
        if (!std::filesystem::exists(assemblyPath))
        {
            HE_LOG_ERROR("Project assembly not found");
            throw std::exception();
        }
        
        Heart::AssetManager::UpdateAssetsDirectory("project");
        Heart::ScriptingEngine::LoadClientPlugin(assemblyPath.u8string());

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
        m_RuntimeScene = Heart::AssetManager::RetrieveAsset<Heart::SceneAsset>(sceneAssetId)->GetScene()->Clone();
        m_RuntimeScene->StartRuntime();

        RuntimeApp::Get().GetWindow().DisableCursor();
    }
}