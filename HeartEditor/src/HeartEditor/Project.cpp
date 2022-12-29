#include "hepch.h"
#include "Project.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "HeartEditor/EditorCamera.h"
#include "HeartEditor/Widgets/ContentBrowser.h"
#include "HeartEditor/Widgets/Viewport.h"
#include "Heart/ImGui/ImGuiInstance.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Util/FilesystemUtils.h"
#include "nlohmann/json.hpp"

namespace HeartEditor
{
    Heart::Ref<Project> Project::CreateAndLoad(const Heart::HStringView8& absolutePath, const Heart::HStringView8& name)
    {
        const char* extension = ".heproj";
        Heart::HString8 filename = name + Heart::HStringView8(extension);

        std::filesystem::path finalPath = std::filesystem::path(absolutePath.Data()).append(name.Data());
        std::filesystem::create_directory(finalPath);
        
        /*
         * Write main project file
         */
        nlohmann::json j;
        j["name"] = name;

        std::filesystem::path mainProjectFilePath = std::filesystem::path(finalPath).append(filename.Data());
        std::ofstream file(mainProjectFilePath);
        file << j;
        file.close();

        /*
         * Create default directories
         */
        std::filesystem::create_directory(std::filesystem::path(finalPath).append("Assets"));
        std::filesystem::create_directory(std::filesystem::path(finalPath).append("Scripts"));
        
        Heart::HString8 scriptsRootPath = std::filesystem::current_path()
            .append("scripting")
            .generic_u8string();
        
        /*
         * Iterate templates directory and perform preprocessing before copying to project dir
         */
        for (const auto& entry : std::filesystem::directory_iterator("templates"))
        {
            Heart::HString8 text = Heart::FilesystemUtils::ReadFileToString(entry.path().generic_u8string());
            text = std::regex_replace(text.Data(), std::regex("\\$\\{SCRIPTS_ROOT_PATH\\}"), scriptsRootPath.Data());
            text = std::regex_replace(text.Data(), std::regex("\\$\\{PROJECT_NAME\\}"), name.Data());
            
            Heart::HString8 filename = entry.path().filename().generic_u8string();
            filename = std::regex_replace(filename.Data(), std::regex("\\$\\{PROJECT_NAME\\}"), name.Data());
            
            auto path = std::filesystem::path(finalPath);
            if (filename == "EmptyEntity.cs")
                path.append("Scripts");
            
            auto file = std::ofstream(
                path.append(filename.Data()),
                std::ios::binary
            );
            file << text.Data();
        }
            
        return LoadFromPath(mainProjectFilePath.generic_u8string());
    }

    Heart::Ref<Project> Project::LoadFromPath(const Heart::HStringView8& absolutePath)
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
            project->m_Name = j["name"];
        
        if (j.contains("loadedScene") && !j["loadedScene"].empty())
        {
            Heart::UUID sceneAssetId = Heart::AssetManager::GetAssetUUID(j["loadedScene"]);
            Editor::OpenSceneFromAsset(sceneAssetId);
        }

        // Parse widgets
        if (j.contains("widgets"))
        {
            auto& field = j["widgets"];
            for (auto& pair : Editor::GetWindows())
                if (field.contains(pair.first.Data()))
                    pair.second->Deserialize(field[pair.first.Data()]);
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
                    field[pair.first.Data()] = serialized;
            }
        }

        std::ofstream file(m_AbsolutePath.Data());
        file << j;
    }

    void Project::LoadScriptsPlugin()
    {
        auto assemblyPath = std::filesystem::path(Heart::AssetManager::GetAssetsDirectory().Data())
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

    void Project::BuildScripts()
    {
        auto timer = Heart::Timer("Client plugin build");
        
        Heart::HString8 command = "cd ";
        command += Heart::AssetManager::GetAssetsDirectory();
        #ifdef HE_PLATFORM_WINDOWS
            command += ";BuildScripts.bat";
        #else
            command += ";sh BuildScripts.sh";
        #endif
        
        int res = system(command.Data());
        if (res == 0)
            HE_ENGINE_LOG_INFO("Client plugin built successfully");
        else
            HE_ENGINE_LOG_ERROR("Client plugin failed to build with code {0}", res);
    }
}
