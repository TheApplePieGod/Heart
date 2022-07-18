#include "hepch.h"
#include "Project.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "HeartEditor/EditorCamera.h"
#include "HeartEditor/Widgets/ContentBrowser.h"
#include "HeartEditor/Widgets/Viewport.h"
#include "Heart/ImGui/ImGuiInstance.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Util/FilesystemUtils.h"
#include "nlohmann/json.hpp"

namespace HeartEditor
{
    Heart::Ref<Project> Project::s_ActiveProject = nullptr;

    Heart::Ref<Project> Project::CreateAndLoad(const std::string& absolutePath, const std::string& name)
    {
        const char* extension = ".heproj";
        std::string filename = name + extension;

        std::filesystem::path finalPath = std::filesystem::path(absolutePath).append(name);
        std::filesystem::create_directory(finalPath);
        
        /*
         * Write main project file
         */
        nlohmann::json j;
        j["name"] = name;

        std::filesystem::path mainProjectFilePath = std::filesystem::path(finalPath).append(filename);
        std::ofstream file(mainProjectFilePath);
        file << j;
        file.close();

        /*
         * Load templates and create visual studio project files
         */
        // This is a temporary solution that will work with versions of the engine built from source. In the future,
        // this will likely have to change
        std::string coreProjectPath = std::filesystem::current_path()
            .parent_path()
            .parent_path()
            .append("HeartScripting")
            .append("CoreScripts.csproj")
            .generic_u8string();

        // Csproj
        std::string csprojTemplate = Heart::FilesystemUtils::ReadFileToString("templates/ProjectTemplate.csproj");
        std::string finalCsproj = std::regex_replace(csprojTemplate, std::regex("\\$\\{CORE_PROJECT_PATH\\}"), coreProjectPath);
        file = std::ofstream(std::filesystem::path(finalPath).append(name + ".csproj"));
        file << finalCsproj;
        file.close();

        // Sln
        std::string slnTemplate = Heart::FilesystemUtils::ReadFileToString("templates/ProjectTemplate.sln");
        std::string finalSln = std::regex_replace(slnTemplate, std::regex("\\$\\{CORE_PROJECT_PATH\\}"), coreProjectPath);
        finalSln = std::regex_replace(finalSln, std::regex("\\$\\{PROJECT_NAME\\}"), name);
        file = std::ofstream(std::filesystem::path(finalPath).append(name + ".sln"));
        file << finalSln;
        file.close();

        /*
         * Copy default imgui config
         */
        std::filesystem::copy_file("templates/imgui.ini", std::filesystem::path(finalPath).append("imgui.ini"));
        
        /*
         * Create default directories
         */
        std::filesystem::create_directory(std::filesystem::path(finalPath).append("Assets"));
        std::filesystem::create_directory(std::filesystem::path(finalPath).append("Scripts"));
        
        return LoadFromPath(mainProjectFilePath.generic_u8string());
    }

    Heart::Ref<Project> Project::LoadFromPath(const std::string& absolutePath)
    {
        Heart::Ref<Project> project = Heart::CreateRef<Project>(absolutePath);
        
        u32 fileLength;
        unsigned char* data = Heart::FilesystemUtils::ReadFile(absolutePath, fileLength);
        if (!data)
            throw std::exception();

        // Clear active scene
        Editor::ClearScene();

        // Finally update the assets directory to the project root
        Heart::AssetManager::UpdateAssetsDirectory(
            Heart::FilesystemUtils::GetParentDirectory(absolutePath)
        );

        // Refresh content browser directory list
        static_cast<Widgets::ContentBrowser&>(
            Editor::GetWindow("Content Browser")
        ).RefreshList();

        // Update the imgui project config
        EditorApp::Get().GetImGuiInstance().OverrideImGuiConfig(Heart::AssetManager::GetAssetsDirectory());
        EditorApp::Get().GetImGuiInstance().ReloadImGuiConfig();

        auto j = nlohmann::json::parse(data);

        if (j.contains("name"))
            project->m_Name = j["name"];
        
        if (j.contains("loadedProject") && j["loadedProject"] != "")
        {
            Heart::UUID sceneAssetId = Heart::AssetManager::GetAssetUUID(j["loadedProject"]);
            Editor::OpenSceneFromAsset(sceneAssetId);
        }

        // Parse widgets
        if (j.contains("widgets"))
        {
            auto& field = j["widgets"];
            for (auto& pair : Editor::GetWindows())
                if (field.contains(pair.first))
                    pair.second->Deserialize(field[pair.first]);
        }

        // Load client scripts if they exist
        project->LoadClientAssembly();

        s_ActiveProject = project;
        delete[] data;
        return project;
    }

    void Project::SaveToDisk()
    {
        nlohmann::json j;
        j["name"] = m_Name;

        Heart::UUID activeSceneAsset = Editor::GetEditorSceneAsset();
        j["loadedProject"] = Heart::AssetManager::GetPathFromUUID(activeSceneAsset);

        // Widget data
        {
            auto& field = j["widgets"];

            for (auto& pair : Editor::GetWindows())
            {
                nlohmann::json serialized = pair.second->Serialize();
                if (!serialized.empty())
                    field[pair.first] = serialized;
            }
        }

        std::ofstream file(m_AbsolutePath);
        file << j;
    }

    void Project::LoadClientAssembly()
    {
        auto assemblyPath = std::filesystem::path(Heart::AssetManager::GetAssetsDirectory())
            .append("bin")
            .append("ClientScripts.dll");
        if (!std::filesystem::exists(assemblyPath))
        {
            HE_LOG_WARN("Client assembly for '{0}' not found", m_Name);
            return;
        }

        if (Heart::ScriptingEngine::LoadClientAssembly(assemblyPath.u8string()))
            HE_LOG_INFO("Client assembly loaded");
    }
}