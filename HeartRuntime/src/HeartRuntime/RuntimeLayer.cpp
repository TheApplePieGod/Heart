#include "hepch.h"
#include "RuntimeLayer.h"

#include "HeartRuntime/RuntimeApp.h"
#include "Heart/Core/Window.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Util/FilesystemUtils.h"
#include "nlohmann/json.hpp"

namespace HeartRuntime
{
    RuntimeLayer::RuntimeLayer()
    {

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

        HE_LOG_INFO("Runtime detached");
    }

    void RuntimeLayer::OnUpdate(Heart::Timestep ts)
    {
        m_RuntimeScene->OnUpdateRuntime(ts);
    }

    void RuntimeLayer::OnImGuiRender()
    {
        m_Viewport.OnImGuiRender(m_RuntimeScene.get());
    }

    void RuntimeLayer::OnEvent(Heart::Event& event)
    {
        event.Map<Heart::KeyPressedEvent>(HE_BIND_EVENT_FN(RuntimeLayer::KeyPressedEvent));
    }

    bool RuntimeLayer::KeyPressedEvent(Heart::KeyPressedEvent& event)
    {
        if (event.GetKeyCode() == Heart::KeyCode::F11)
            RuntimeApp::Get().GetWindow().ToggleFullscreen();
        
        return true;
    }

    void RuntimeLayer::LoadProject()
    {
        // Locate the project file to run
        auto projectFolderPath = std::filesystem::path("project");
        if (!std::filesystem::exists(projectFolderPath))
        {
            HE_LOG_ERROR("Unable to locate project folder");
            throw std::exception();
        }

        auto projectPath = std::filesystem::path();
        for (const auto& entry : std::filesystem::directory_iterator(projectFolderPath))
        {
            if (!entry.is_directory() && entry.path().extension() == ".heproj")
            {
                projectPath = entry.path();
                break;
            }
        }

        if (projectPath.empty())
        {
            HE_LOG_ERROR("Unable to locate project");
            throw std::exception();
        }

        u32 fileLength;
        unsigned char* data = Heart::FilesystemUtils::ReadFile(projectPath.generic_u8string(), fileLength);
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

        // TODO: eventually switch from loadedScene to default scene or something like that
        auto j = nlohmann::json::parse(data);
        if (!j.contains("loadedScene") || j["loadedScene"].empty())
        {
            HE_LOG_ERROR("Unable to load scene");
            throw std::exception();
        }

        if (j.contains("name") && !j["name"].empty())
        {
            std::string title = j["name"];
            RuntimeApp::Get().GetWindow().SetWindowTitle(title.c_str());
        }

        Heart::UUID sceneAssetId = Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Scene, j["loadedScene"]);
        m_RuntimeScene = Heart::AssetManager::RetrieveAsset<Heart::SceneAsset>(sceneAssetId)->GetScene()->Clone();
        m_RuntimeScene->StartRuntime();

        RuntimeApp::Get().GetWindow().DisableCursor();
    }
}