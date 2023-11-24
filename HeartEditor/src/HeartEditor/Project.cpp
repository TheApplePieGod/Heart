#include "hepch.h"
#include "Project.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "HeartEditor/EditorCamera.h"
#include "HeartEditor/Widgets/ContentBrowser.h"
#include "HeartEditor/Widgets/MaterialEditor.h"
#include "HeartEditor/Widgets/Viewport.h"
#include "Heart/ImGui/ImGuiInstance.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Util/PlatformUtils.h"
#include "nlohmann/json.hpp"

namespace HeartEditor
{
    Heart::Ref<Project> Project::CreateAndLoad(const Heart::HStringView8& absolutePath, const Heart::HStringView8& name)
    {
        HE_PROFILE_FUNCTION();

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
        HE_PROFILE_FUNCTION();

        Heart::Ref<Project> project = Heart::CreateRef<Project>(absolutePath);
        
        u32 fileLength;
        unsigned char* data = Heart::FilesystemUtils::ReadFile(absolutePath, fileLength);
        if (!data)
            throw std::exception();

        // Cleanup editor state
        Editor::ClearScene();
        
        // Finally update the assets directory to the project root
        Heart::AssetManager::UpdateAssetsDirectory(
            Heart::FilesystemUtils::GetParentDirectory(absolutePath)
        );

        ((Widgets::ContentBrowser&)Editor::GetWindow("Content Browser")).Reset();
        ((Widgets::MaterialEditor&)Editor::GetWindow("Material Editor")).Reset();
        
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
        HE_PROFILE_FUNCTION();

        auto assemblyPath = std::filesystem::path(Heart::AssetManager::GetAssetsDirectory().Data())
            .append("bin")
            .append("ClientScripts.dll");
        if (!std::filesystem::exists(assemblyPath))
        {
            HE_LOG_WARN("Client assembly not found");
            return;
        }

        const auto serializeInstance = [](nlohmann::json& j, Heart::ScriptInstance* instance)
        {
            if (!instance->IsInstantiable() || !instance->IsAlive())
                return false;

            j["type"] = instance->GetScriptClassObject().GetFullName();
            j["fields"] = instance->SerializeFieldsToJson();

            return true;
        };

        // Serialize object state of all alive scripts
        // TODO: stop using json here because it is slow
        std::unordered_map<entt::entity, SerializedInstance> serializedObjects;
        for (auto [_entity] : Editor::GetActiveScene().GetEntityIterator())
        {
            Heart::Entity entity(&Editor::GetActiveScene(), (u32)_entity);

            SerializedInstance instance;
            bool include = false;

            // Check for default script component
            if (entity.HasComponent<Heart::ScriptComponent>())
            {
                include |= serializeInstance(
                    instance.ScriptComp,
                    &entity.GetComponent<Heart::ScriptComponent>().Instance
                );
            }

            // Check for any runtime components
            for (auto& pair : Heart::ScriptingEngine::GetComponentClasses())
            {
                if (!entity.HasRuntimeComponent(pair.first)) continue;

                nlohmann::json j;
                if (serializeInstance(j, &entity.GetRuntimeComponent(pair.first).Instance))
                {
                    instance.RuntimeComps[pair.first] = j;
                    include = true;
                }
            }

            if (include)
                serializedObjects[_entity] = instance;
        }

        // Reload
        #ifdef HE_DEBUG
            Heart::ScriptingEngine::ReloadCorePlugin();
        #endif
        Heart::ScriptingEngine::LoadClientPlugin(assemblyPath.u8string());

        // Reinstantiate objects and load serialized properties
        for (auto& pair : serializedObjects)
        {
            Heart::Entity entity(&Editor::GetActiveScene(), (u32)pair.first);
            auto& instance = pair.second;
            
            // Instantiate script component
            if (!instance.ScriptComp.is_null())
            {
                auto& scriptComp = entity.GetComponent<Heart::ScriptComponent>();

                // We need to ensure that the class still exists & is instantiable after the reload
                // before we try and reinstantiate it
                scriptComp.Instance.ClearObjectHandle();
                if (!scriptComp.Instance.IsInstantiable())
                {
                    HE_ENGINE_LOG_WARN(
                        "Script class '{0}' referenced in entity is no longer instantiable (id: {1}, name: {2})",
                        instance.ScriptComp["type"],
                        entity.GetUUID(),
                        entity.GetName().DataUTF8()
                    );
                    continue;
                }

                // Reinstantiate
                scriptComp.Instance.Instantiate(entity);
                scriptComp.Instance.LoadFieldsFromJson(instance.ScriptComp["fields"]);
                scriptComp.Instance.OnConstruct();
            }

            // Instantiate runtime components
            for (auto& pair : instance.RuntimeComps)
            {
                auto& comp = entity.GetRuntimeComponent(pair.first);
                auto& compJ = pair.second;

                // We need to ensure that the class still exists & is instantiable after the reload
                // before we try and reinstantiate it
                comp.Instance.ClearObjectHandle();
                if (!comp.Instance.IsInstantiable())
                {
                    HE_ENGINE_LOG_WARN(
                        "Component class '{0}' referenced in entity is no longer instantiable (id: {1}, name: {2})",
                        compJ["type"],
                        entity.GetUUID(),
                        entity.GetName().DataUTF8()
                    );

                    // Remove component since it no longer makes sene to keep it
                    entity.RemoveRuntimeComponent(pair.first);

                    continue;
                }

                // Reinstantiate
                comp.Instance.Instantiate();
                comp.Instance.LoadFieldsFromJson(compJ["fields"]);
            }
        }

        // Ensure potential modifications of OnConstruct are reflected
        Editor::GetActiveScene().CacheDirtyTransforms();
    }

    bool Project::BuildScripts(bool debug)
    {
        HE_PROFILE_FUNCTION();

        auto timer = Heart::Timer("Client plugin build");
        
        #ifdef HE_PLATFORM_WINDOWS
            Heart::HString8 command = "BuildScripts.bat ";
        #else
            Heart::HString8 command = "sh BuildScripts.sh ";
        #endif
        
        if (debug)
            command += "Debug";
        else
            command += "Release";

        int res;
        Heart::HString8 output = RunCommandInProjectDirectory(command, res);
        if (res == 0)
        {
            HE_ENGINE_LOG_INFO("Client plugin built successfully");
            HE_ENGINE_LOG_INFO("Build output: {0}", output.Data());
        }
        else
        {
            HE_ENGINE_LOG_ERROR("Client plugin failed to build with code {0}", res);
            HE_ENGINE_LOG_ERROR("Build output: {0}", output.Data());
        }
        
        return res == 0;
    }

    bool Project::PublishScripts(ExportPlatform platform)
    {
        HE_PROFILE_FUNCTION();

        auto timer = Heart::Timer("Client plugin publish");
        
        #ifdef HE_PLATFORM_WINDOWS
            Heart::HString8 command = "PublishScripts.bat ";
        #else
            Heart::HString8 command = "sh PublishScripts.sh ";
        #endif
        
        auto runtimeId = GetDotnetRuntimeIdentifier(platform);
        if (runtimeId.IsEmpty())
        {
            HE_LOG_ERROR("Unsupported publish platform '{0}'", (u8)platform);
            return false;
        }
        command += runtimeId;

        int res;
        Heart::HString8 output = RunCommandInProjectDirectory(command, res);
        if (res == 0)
        {
            HE_LOG_INFO("Client plugin published successfully");
            HE_LOG_INFO("Publish output: {0}", output.Data());
        }
        else
        {
            HE_LOG_ERROR("Client plugin failed to publish with code {0}", res);
            HE_LOG_ERROR("Publish output: {0}", output.Data());
        }
        
        return res == 0;
    }
    
    void Project::Export(Heart::HStringView8 absolutePath, ExportPlatform platform)
    {
        HE_PROFILE_FUNCTION();

        if (!PublishScripts(platform))
        {
            HE_ENGINE_LOG_ERROR("Failed to export project, client plugin publish failed");
            return;
        }

        std::filesystem::path finalPath = std::filesystem::path(absolutePath.Data()).append(m_Name.Data());
        std::filesystem::create_directory(finalPath);
        
        #ifdef HE_PLATFORM_MACOS
            Heart::HStringView8 runtimeExt = ".app";
        #else
            Heart::HStringView8 runtimeExt = ".exe";
        #endif
        
        Heart::HString8 runtimeName = Heart::HStringView8("Runtime") + runtimeExt;
        Heart::HString8 finalName = m_Name + runtimeExt;

        if (!std::filesystem::exists(runtimeName.Data()))
        {
            HE_ENGINE_LOG_ERROR("Failed to export project, missing runtime executable for platform");
            return;
        }

        std::filesystem::copy(
            runtimeName.Data(),
            std::filesystem::path(finalPath).append(finalName.Data()),
            std::filesystem::copy_options::recursive
        );
        
        std::filesystem::path copyPath;
        std::filesystem::path engineResources = std::filesystem::path("resources").append("engine");
        #ifdef HE_PLATFORM_MACOS
            // Copy files to bundle resources directory
            copyPath = std::filesystem::path(finalPath).append(finalName.Data());
            copyPath.append("Contents");
            std::filesystem::create_directory(copyPath);
            copyPath.append("Resources");
            std::filesystem::create_directory(copyPath);
        #else
            copyPath = std::filesystem::path(finalPath);
        #endif

        copyPath.append("resources");
        std::filesystem::create_directory(copyPath);
        copyPath.append("engine");
        std::filesystem::create_directory(copyPath);
        std::filesystem::copy(engineResources, copyPath, std::filesystem::copy_options::recursive);
            
        auto runtimeId = GetDotnetRuntimeIdentifier(platform);
        copyPath = copyPath.parent_path().parent_path().append("scripting");
        std::filesystem::create_directory(copyPath);
        std::filesystem::copy(
            std::filesystem::path(m_AbsolutePath.Data()).parent_path().append("bin").append(runtimeId.Data()).append("publish"),
            copyPath,
            std::filesystem::copy_options::recursive
        );
        
        copyPath = copyPath.parent_path().append("project");
        std::filesystem::create_directory(copyPath);
        std::filesystem::copy(
            std::filesystem::path(m_AbsolutePath.Data()).parent_path(),
            copyPath,
            std::filesystem::copy_options::recursive
        );
        
        // Copy extra files
        #ifdef HE_PLATFORM_MACOS
            std::filesystem::path dst = std::filesystem::path(copyPath).parent_path().parent_path();
            dst.append("Frameworks");
            std::filesystem::create_directory(dst);
            std::filesystem::copy(
              "libMoltenVK.dylib",
              std::filesystem::path(dst).append("libMoltenVK.dylib")
            );
            // TODO: this is bad. We should be using a symlink, but it doesn't seem to work, so for now
            // we are just copying the entire dylib again
            std::filesystem::copy(
              "libMoltenVK.dylib",
              std::filesystem::path(dst).append("libvulkan.1.dylib")
            );

            std::filesystem::copy(
              "libshaderc_shared.1.dylib",
              std::filesystem::path(dst).append("libshaderc_shared.1.dylib")
            );
            std::filesystem::copy(
              "libspirv-cross-c-shared.0.dylib",
              std::filesystem::path(dst).append("libspirv-cross-c-shared.0.dylib")
            );
        #elif defined(HE_PLATFORM_WINDOWS)
            std::filesystem::path dst = std::filesystem::path(copyPath).parent_path();
            std::filesystem::copy(
              "shaderc_shared.dll",
              std::filesystem::path(dst).append("shaderc_shared.dll")
            );
            std::filesystem::copy(
              "spirv-cross-c-shared.dll",
              std::filesystem::path(dst).append("spirv-cross-c-shared.dll")
            );
        #endif

        // Generate assets manifest
        auto assetsManifest = Heart::AssetManager::GenerateManifest();
        dst = std::filesystem::path(copyPath).append(Heart::AssetManager::GetManifestFilename().Data());
        Heart::FilesystemUtils::WriteFile(dst.generic_u8string(), assetsManifest);

        // Write metadata
        nlohmann::json metadata;
        metadata["projectName"] = m_Name;
        dst = std::filesystem::path(copyPath).parent_path().append("metadata.json");
        Heart::FilesystemUtils::WriteFile(dst.generic_u8string(), metadata);
        
        HE_ENGINE_LOG_INFO("Project exported successfully to {0}", finalPath.generic_u8string());
    }

    Heart::HString8 Project::RunCommandInProjectDirectory(Heart::HStringView8 command, int& outResult)
    {
        #ifdef HE_PLATFORM_WINDOWS
            Heart::HString8 prefix = "/k cd ";
        #else
            Heart::HString8 prefix = "cd ";
        #endif

        prefix += Heart::AssetManager::GetAssetsDirectory();
        
        #ifdef HE_PLATFORM_WINDOWS
            prefix += " && ";
        #else
            prefix += ";";
        #endif

        prefix += command;
        
        Heart::HString8 output;
        outResult = Heart::PlatformUtils::ExecuteCommandWithOutput(prefix, output);
        return output;
    }

    Heart::HStringView8 Project::GetDotnetRuntimeIdentifier(ExportPlatform platform)
    {
        switch (platform)
        {
            default: { return Heart::HStringView8(); }
            case ExportPlatform::Windows: { return "win-x64"; }
            case ExportPlatform::MacOS: { return "osx-arm64"; }
            case ExportPlatform::Linux: { return "linux-x64"; }
            case ExportPlatform::Android: { return "android-arm64"; }
        }
    }
}
