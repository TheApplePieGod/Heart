#include "hepch.h"
#include "Editor.h"

#include "HeartEditor/EditorApp.h"
#include "HeartEditor/Widgets/Widget.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Util/FilesystemUtils.h"
#include "imgui/imgui.h"

#include "HeartEditor/Widgets/SceneHierarchyPanel.h"
#include "HeartEditor/Widgets/PropertiesPanel.h"
#include "HeartEditor/Widgets/ContentBrowser.h"
#include "HeartEditor/Widgets/MaterialEditor.h"
#include "HeartEditor/Widgets/Viewport.h"
#include "HeartEditor/Widgets/DebugInfo.h"
#include "HeartEditor/Widgets/ProjectSettings.h"
#include "HeartEditor/Widgets/SceneSettings.h"
#include "HeartEditor/Widgets/RenderSettings.h"
#include "HeartEditor/Widgets/AssetRegistry.h"
#include "HeartEditor/Widgets/ShaderRegistry.h"
#include "HeartEditor/Widgets/LogList.h"
#include "HeartEditor/Widgets/RenderGraph.h"

namespace HeartEditor
{
    void Editor::Initialize()
    {
        HE_PROFILE_FUNCTION();

        ReloadEditorConfig();

        s_EditorScene = Heart::CreateRef<Heart::Scene>();
        s_ActiveScene = s_EditorScene;

        GetState().RenderSettings.CopyEntityIdsTextureToCPU = true;
        GetState().RenderSettings.AsyncAssetLoading = true;
        GetState().RenderSettings.RenderPhysicsVolumes = false;
    }

    void Editor::Shutdown()
    {
        s_Windows.clear();
        s_ActiveScene.reset();
        s_EditorScene.reset();
        s_RenderScene.Cleanup();
    }

    void Editor::OnUpdate(Heart::Timestep ts)
    {
        // Iterate the serial queue
        s_SerialQueueLock.lock();
        for (const auto& func : s_SerialQueue)
            func();
        s_SerialQueue.Clear();
        s_SerialQueueLock.unlock();

        // Tick status elements
        s_StatusLock.lock();
        for (u32 i = 0; i < s_EditorState.StatusElements.Count(); i++)
        {
            auto& elem = s_EditorState.StatusElements[i];

            if (elem.Duration <= 0.f)
                s_EditorState.StatusElements.Remove(i--);

            // Subtract after so that we get one extra frame on screen
            elem.Duration -= ts.StepMilliseconds();
        }
        s_StatusLock.unlock();
    }

    void Editor::CreateWindows()
    {
        Heart::TaskGroup taskGroup;

        taskGroup.AddTask(PushWindow<Widgets::Viewport>("Viewport", true));
        taskGroup.AddTask(PushWindow<Widgets::ContentBrowser>("Content Browser", true));
        taskGroup.AddTask(PushWindow<Widgets::SceneHierarchyPanel>("Scene Hierarchy", true));
        taskGroup.AddTask(PushWindow<Widgets::PropertiesPanel>("Properties Panel", true));
        taskGroup.AddTask(PushWindow<Widgets::MaterialEditor>("Material Editor", false));
        taskGroup.AddTask(PushWindow<Widgets::DebugInfo>("Debug Info", true));
        taskGroup.AddTask(PushWindow<Widgets::ProjectSettings>("Project Settings", true));
        taskGroup.AddTask(PushWindow<Widgets::SceneSettings>("Scene Settings", true));
        taskGroup.AddTask(PushWindow<Widgets::RenderSettings>("Render Settings", true));
        taskGroup.AddTask(PushWindow<Widgets::AssetRegistry>("Asset Registry", false));
        taskGroup.AddTask(PushWindow<Widgets::ShaderRegistry>("Shader Registry", false));
        taskGroup.AddTask(PushWindow<Widgets::LogList>("Log List", false));
        taskGroup.AddTask(PushWindow<Widgets::RenderGraph>("Render Graph", false));

        taskGroup.Wait();
    }

    void Editor::DestroyWindows()
    {
        s_Windows.clear();
    }

    void Editor::RenderWindows()
    {
        HE_PROFILE_FUNCTION();
        auto timer = Heart::AggregateTimer("Editor::RenderWindows");

        for (auto& pair : s_Windows)
            pair.second->OnImGuiRender();
    }

    void Editor::RenderWindowsPostSceneUpdate()
    {
        HE_PROFILE_FUNCTION();
        auto timer = Heart::AggregateTimer("Editor::RenderWindowsPostSceneUpdate");

        for (auto& pair : s_Windows)
            pair.second->OnImGuiRenderPostSceneUpdate();

        if (s_ImGuiDemoOpen)
            ImGui::ShowDemoWindow(&s_ImGuiDemoOpen);
    }

    void Editor::ReloadEditorConfig()
    {
        std::filesystem::path homeDir(Heart::FilesystemUtils::GetHomeDirectory().Data());
        if (homeDir.empty())
        {
            HE_LOG_ERROR("Could not find HOME directory, falling back to default editor config");
            return;
        }

        auto configDir = homeDir.append(s_ConfigDirectoryName.Data());
        if (!std::filesystem::exists(configDir))
            std::filesystem::create_directory(configDir);
        s_ConfigDirectory = configDir.generic_u8string();

        auto configFile = configDir.append(s_ConfigFileName.Data());
        if (!std::filesystem::exists(configFile))
        {
            HE_LOG_INFO("Could not find global editor config, falling back to default editor config");
            return;
        }

        nlohmann::json config = Heart::FilesystemUtils::ReadFileToJson(configFile.generic_u8string());
        if (config.contains("recentProjects"))
        {
            for (auto& project : config["recentProjects"])
            {
                ProjectDescriptor entry;
                entry.Path = project["path"];
                entry.Name = project["name"];

                s_EditorConfig.RecentProjects.AddInPlace(entry);
            }
        }

        HE_LOG_INFO("Loaded editor config from '{}'", s_ConfigDirectory.Data());
    }

    void Editor::SaveEditorConfig()
    {
        if (s_ConfigDirectory.IsEmpty()) return;

        nlohmann::json j;
        
        u32 projectIndex = 0;
        auto& recentProjects = j["recentProjects"];
        for (auto& project : s_EditorConfig.RecentProjects)
        {
            nlohmann::json entry;
            entry["path"] = project.Path;
            entry["name"] = project.Name;

            recentProjects[projectIndex++] = entry;
        }
        
        auto configPath = std::filesystem::path(s_ConfigDirectory.Data()).append(s_ConfigFileName.Data());
        Heart::FilesystemUtils::WriteFile(configPath.generic_u8string(), j);

        HE_LOG_TRACE("Saving editor config at {}", configPath.generic_u8string().c_str());
    }

    void Editor::SetActiveProject(Heart::Ref<Project>& project)
    {
        s_EditorState.ActiveProject = project;

        // Update recent project list
        for (u32 i = 0; i < s_EditorConfig.RecentProjects.Count(); i++)
        {
            if (s_EditorConfig.RecentProjects[i].Path == project->GetPath())
            {
                // Remove so that we can re-add in the front of the list
                s_EditorConfig.RecentProjects.Remove(i);
                break;
            }
        }

        ProjectDescriptor entry;
        entry.Path = Heart::HString8(project->GetPath());
        entry.Name = Heart::HString8(project->GetName());
        s_EditorConfig.RecentProjects.Insert(entry, 0);

        SaveEditorConfig();
    }

    void Editor::SaveProject()
    {
        // TODO: can this actually fail?
        s_EditorState.ActiveProject->SaveToDisk();

        StatusElement buildStatus;
        buildStatus.Duration = 2000.f;
        buildStatus.Type = StatusElementType::Success;
        buildStatus.Text = "Project saved";
        Editor::PushStatus(buildStatus);
    }

    void Editor::SaveScene()
    {
        if (!s_EditorSceneAsset) return;

        StatusElement buildStatus;
        buildStatus.Duration = 2000.f;
        
        auto asset = Heart::AssetManager::RetrieveAsset<Heart::SceneAsset>(s_EditorSceneAsset);
        if (asset)
        {
            // TODO: can this actually fail?
            asset->Save(s_EditorScene.get());

            buildStatus.Type = StatusElementType::Success;
            buildStatus.Text = "Scene saved";
            Editor::PushStatus(buildStatus);
        }
        else
        {
            buildStatus.Type = StatusElementType::Error;
            buildStatus.Text = "Failed to save scene";
            Editor::PushStatus(buildStatus);
        }
    }

    void Editor::OpenScene(const Heart::Ref<Heart::Scene>& scene)
    {
        if (s_SceneState != SceneState::Editing)
            StopScene();

        s_ActiveScene = scene;
        s_EditorScene = scene;
        s_EditorState.SelectedEntity = Heart::Entity();
        s_EditorSceneAsset = 0; // Active scene cannot assume there will be a related asset

        // int maxPos = 25;
        // for (int i = 0; i < 500; i++)
        // {
        //     Heart::Entity entity = s_ActiveScene->CreateEntity("ScriptEntity " + std::to_string(i));
        //     entity.AddComponent<Heart::ScriptComponent>().Instance = Heart::ScriptInstance("TestProject.Scripts.TestEntity");
        //     entity.AddComponent<Heart::MeshComponent>().Mesh = Heart::AssetManager::GetAssetUUID("Assets/Meshes/Avocado/glTF/Avocado.gltf");
        //     entity.SetPosition({ rand() % (maxPos * 2) - maxPos, rand() % (maxPos * 2) - maxPos, rand() % (maxPos * 2) - maxPos });
        //     entity.SetScale({ 15.f, 15.f, 15.f });
        // }
    }

    void Editor::OpenSceneFromAsset(Heart::UUID uuid)
    {
        auto sceneAsset = Heart::AssetManager::RetrieveAsset<Heart::SceneAsset>(uuid);
        if (!sceneAsset || !sceneAsset->Load()->IsValid())
        {
            HE_ENGINE_LOG_ERROR("Failed to load scene");
            return;
        }
        
        OpenScene(sceneAsset->GetScene());

        sceneAsset->Unload();

        s_EditorSceneAsset = uuid;
    }

    void Editor::ClearScene()
    {
        auto emptyScene = Heart::CreateRef<Heart::Scene>();
        OpenScene(emptyScene);
    }

    void Editor::PlayScene()
    {
        s_SceneState = SceneState::Playing;

        s_ActiveScene = s_EditorScene->Clone();
        s_ActiveScene->StartRuntime();

        s_EditorState.SelectedEntity = Heart::Entity();

        auto& viewport = (Widgets::Viewport&)GetWindow("Viewport");
        if (viewport.ShouldCameraAttach())
            viewport.SetFocused(true);
    }

    void Editor::StopScene()
    {
        s_SceneState = SceneState::Editing;

        s_SceneUpdateTask.Wait();
        
        s_ActiveScene->StopRuntime();
        s_ActiveScene = s_EditorScene;

        s_EditorState.SelectedEntity = Heart::Entity();
        
        auto& viewport = (Widgets::Viewport&)GetWindow("Viewport");
        viewport.ResetEditorCamera();
    }

    void Editor::StartScriptCompilation()
    {
        if (s_EditorState.IsCompilingScripts) return;

        s_EditorState.IsCompilingScripts = true;

        StatusElement buildStatus;
        buildStatus.Duration = std::numeric_limits<f32>::max();
        buildStatus.Type = StatusElementType::Loading;
        buildStatus.Text = "Compiling scripts...";
        Editor::PushStatus(buildStatus);

        // Create a task so the compilation is non-blocking
        Heart::UUID statusId = buildStatus.Id;
        Heart::TaskManager::Schedule(
            [statusId]()
            {
                bool success = s_EditorState.ActiveProject->BuildScripts(true);

                // Finalize in serial to prevent issues reloading the plugin
                PushSerialQueue([success, statusId](){
                    if (success)
                        s_EditorState.ActiveProject->LoadScriptsPlugin();

                    s_EditorState.IsCompilingScripts = false;

                    StatusElement buildStatus;
                    buildStatus.Id = statusId;
                    buildStatus.Duration = 4000.f;
                    buildStatus.Type = success ? StatusElementType::Success : StatusElementType::Error;
                    buildStatus.Text = success ? "Compilation successful!" : "Compilation failed, check logs";
                    UpdateStatus(buildStatus);
                });
            },
            Heart::Task::Priority::Medium,
            "Script compilation"
        );
    }

    bool Editor::IsDirty()
    {
        for (auto& pair : s_Windows)
            if (pair.second->IsDirty())
                return true;
        return false;
    }

    void Editor::PushSerialQueue(std::function<void()>&& func)
    {
        std::lock_guard lock(s_SerialQueueLock);
        s_SerialQueue.Add(func);
    }

    void Editor::PushStatus(const StatusElement& elem)
    {
        switch (elem.Type)
        {
            default: break;
            case StatusElementType::Info: { HE_LOG_INFO("{}", elem.Text.Data()); } break;
            case StatusElementType::Warn: { HE_LOG_WARN("{}", elem.Text.Data()); } break;
            case StatusElementType::Error: { HE_LOG_ERROR("{}", elem.Text.Data()); } break;
        }

        std::lock_guard lock(s_StatusLock);
        s_EditorState.StatusElements.Add(elem);
    }

    void Editor::UpdateStatus(const StatusElement& newStatus)
    {
        std::lock_guard lock(s_StatusLock);

        for (u32 i = 0; i < s_EditorState.StatusElements.Count(); i++)
        {
            if (s_EditorState.StatusElements[i].Id == newStatus.Id)
            {
                s_EditorState.StatusElements[i] = newStatus;
                return;
            }
        }
    }
}
