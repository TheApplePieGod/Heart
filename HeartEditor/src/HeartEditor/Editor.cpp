#include "hepch.h"
#include "Editor.h"

#include "HeartEditor/EditorApp.h"
#include "HeartEditor/Widgets/Widget.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Renderer/SceneRenderer.h"
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

        s_EditorScene = Heart::CreateRef<Heart::Scene>();
        s_ActiveScene = s_EditorScene;

        GetState().RenderSettings.CopyEntityIdsTextureToCPU = true;
        GetState().RenderSettings.AsyncAssetLoading = true;
        GetState().RenderSettings.RenderPhysicsVolumes = false;
        
        CreateWindows();
    }

    void Editor::Shutdown()
    {
        s_Windows.clear();
        s_ActiveScene.reset();
        s_EditorScene.reset();
        s_RenderScene.Cleanup();
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

    void Editor::SaveScene()
    {
        if (!s_EditorSceneAsset) return;
        
        auto asset = Heart::AssetManager::RetrieveAsset<Heart::SceneAsset>(s_EditorSceneAsset);
        if (asset)
            asset->Save(s_EditorScene.get());
        else
            HE_ENGINE_LOG_ERROR("Failed to save scene");
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

    bool Editor::IsDirty()
    {
        for (auto& pair : s_Windows)
            if (pair.second->IsDirty())
                return true;
        return false;
    }
}
