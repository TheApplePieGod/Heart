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
    Heart::Ref<Project> Project::CreateAndLoad(const Heart::HStringView& absolutePath, const Heart::HStringView& name)
    {
        const char* extension = ".heproj";
        Heart::HString filename = name + extension;

        std::filesystem::path finalPath = std::filesystem::path(absolutePath.DataUTF8()).append(name.DataUTF8());
        std::filesystem::create_directory(finalPath);
        
        /*
         * Write main project file
         */
        nlohmann::json j;
        j["name"] = name;

        std::filesystem::path mainProjectFilePath = std::filesystem::path(finalPath).append(filename.DataUTF8());
        std::ofstream file(mainProjectFilePath);
        file << j;
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

        /*
         * Load templates and create visual studio project files
         */
        Heart::HString scriptsRootPath = std::filesystem::current_path()
            .append("scripting")
            .generic_u8string();

        // Csproj
        Heart::HString csprojTemplate = Heart::FilesystemUtils::ReadFileToString("templates/ProjectTemplate.csproj");
        Heart::HString finalCsproj = std::regex_replace(csprojTemplate.DataUTF8(), std::regex("\\$\\{SCRIPTS_ROOT_PATH\\}"), scriptsRootPath.DataUTF8());
        file = std::ofstream(
            std::filesystem::path(finalPath).append((name + ".csproj").DataUTF8()),
            std::ios::binary
        );
        file << finalCsproj.DataUTF8();
        file.close();

        // Sln
        Heart::HString slnTemplate = Heart::FilesystemUtils::ReadFileToString("templates/ProjectTemplate.sln");
        Heart::HString finalSln = std::regex_replace(slnTemplate.DataUTF8(), std::regex("\\$\\{SCRIPTS_ROOT_PATH\\}"), scriptsRootPath.DataUTF8());
        finalSln = (Heart::HString)std::regex_replace(finalSln.DataUTF8(), std::regex("\\$\\{PROJECT_NAME\\}"), name.DataUTF8());
        file = std::ofstream(
            std::filesystem::path(finalPath).append((name + ".sln").DataUTF8()),
            std::ios::binary
        );
        file << finalSln.DataUTF8();
        file.close();

        // Empty entity
        Heart::HString entityTemplate = Heart::FilesystemUtils::ReadFileToString("templates/EmptyEntity.csfile");
        Heart::HString finalEntity = std::regex_replace(entityTemplate.DataUTF8(), std::regex("\\$\\{PROJECT_NAME\\}"), name.DataUTF8());
        file = std::ofstream(
            std::filesystem::path(finalPath).append("Scripts").append("EmptyEntity.cs"),
            std::ios::binary
        );
        file << finalEntity.DataUTF8();
        file.close();
        
        return LoadFromPath(mainProjectFilePath.generic_u8string());
    }

    Heart::Ref<Project> Project::LoadFromPath(const Heart::HStringView& absolutePath)
    {
        Heart::Ref<Project> project = Heart::CreateRef<Project>(absolutePath);
        
        u32 fileLength;
        unsigned char* data = Heart::FilesystemUtils::ReadFile(absolutePath, fileLength);
        if (!data)
            throw std::exception();

        // Cleanup editor state
        Editor::ClearScene();
        Editor::DestroyWindows();

        // Finally update the assets directory to the project root
        Heart::AssetManager::UpdateAssetsDirectory(
            Heart::FilesystemUtils::GetParentDirectory(absolutePath)
        );

        // Recreate editor windows
        Editor::CreateWindows();

        // Update the imgui project config
        EditorApp::Get().GetImGuiInstance().OverrideImGuiConfig(Heart::AssetManager::GetAssetsDirectory());
        EditorApp::Get().GetImGuiInstance().ReloadImGuiConfig();

        // Load client scripts if they exist
        project->LoadScriptsPlugin();

        auto j = nlohmann::json::parse(data);

        if (j.contains("name"))
            project->m_Name = (Heart::HString)j["name"];
        
        if (j.contains("loadedScene") && j["loadedScene"] != "")
        {
            Heart::UUID sceneAssetId = Heart::AssetManager::GetAssetUUID(j["loadedScene"]);
            Editor::OpenSceneFromAsset(sceneAssetId);
        }

        // Parse widgets
        if (j.contains("widgets"))
        {
            auto& field = j["widgets"];
            for (auto& pair : Editor::GetWindows())
                if (field.contains(pair.first.DataUTF8()))
                    pair.second->Deserialize(field[pair.first.DataUTF8()]);
        }

        s_ActiveProject = project;
        delete[] data;
        return project;
    }

    void Project::SaveToDisk()
    {
        nlohmann::json j;
        j["name"] = m_Name;

        Heart::UUID activeSceneAsset = Editor::GetEditorSceneAsset();
        j["loadedScene"] = Heart::AssetManager::GetPathFromUUID(activeSceneAsset);

        // Widget data
        {
            auto& field = j["widgets"];

            for (auto& pair : Editor::GetWindows())
            {
                nlohmann::json serialized = pair.second->Serialize();
                if (!serialized.empty())
                    field[pair.first.DataUTF8()] = serialized;
            }
        }

        std::ofstream file(m_AbsolutePath.DataUTF8());
        file << j;
    }

    void Project::LoadScriptsPlugin()
    {
        auto assemblyPath = std::filesystem::path(Heart::AssetManager::GetAssetsDirectory().DataUTF8())
            .append("bin")
            .append("ClientScripts.dll");
        if (!std::filesystem::exists(assemblyPath))
        {
            HE_LOG_WARN("Client assembly not found");
            return;
        }

        // Serialize object state of all alive scripts
        auto view = Editor::GetActiveScene().GetRegistry().view<Heart::ScriptComponent>();
        std::unordered_map<entt::entity, nlohmann::json> serializedObjects;
        for (auto entity : view)
        {
            auto& scriptComp = view.get<Heart::ScriptComponent>(entity);
            if (!scriptComp.Instance.IsAlive()) continue;
            
            serializedObjects[entity] = scriptComp.Instance.SerializeFieldsToJson();
        }

        // Reload
        Heart::ScriptingEngine::LoadClientPlugin(assemblyPath.u8string());

        // Reinstantiate objects and load serialized properties
        for (auto entity : view)
        {
            auto& scriptComp = view.get<Heart::ScriptComponent>(entity);
            if (!scriptComp.Instance.IsInstantiable())
                continue;
            
            // We need to ensure that the class still exists & is instantiable after the reload
            // before we try and reinstantiate it
            scriptComp.Instance.ClearObjectHandle();
            if (!scriptComp.Instance.ValidateClass())
            {
                HE_ENGINE_LOG_WARN(
                    "Class '{0}' referenced in scene is no longer instantiable",
                    scriptComp.Instance.GetScriptClass().DataUTF8()
                );
                continue;
            }

            // Reinstantiate
            scriptComp.Instance.Instantiate({ &Editor::GetActiveScene(), (u32)entity });
            scriptComp.Instance.LoadFieldsFromJson(serializedObjects[entity]);
        }
    }
}