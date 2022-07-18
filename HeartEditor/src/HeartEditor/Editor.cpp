#include "hepch.h"
#include "Editor.h"

#include "HeartEditor/EditorApp.h"
#include "HeartEditor/Widgets/Widget.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
    void Editor::Initialize()
    {
        s_EditorScene = Heart::CreateRef<Heart::Scene>();
        s_ActiveScene = s_EditorScene;
    }

    void Editor::Shutdown()
    {
        s_Windows.clear();
        s_ActiveScene.reset();
        s_EditorScene.reset();
    }

    void Editor::RenderWindows()
    {
        HE_PROFILE_FUNCTION();
        auto timer = Heart::AggregateTimer("Editor::RenderWindows");

        for (auto& pair : s_Windows)
            pair.second->OnImGuiRender();

        if (s_ImGuiDemoOpen)
            ImGui::ShowDemoWindow(&s_ImGuiDemoOpen);
    }

    void Editor::OpenScene(const Heart::Ref<Heart::Scene>& scene)
    {
        if (s_SceneState != SceneState::Editing)
            StopScene();

        s_ActiveScene = scene;
        s_EditorScene = scene;
        s_EditorState.SelectedEntity = Heart::Entity();
        s_EditorSceneAsset = 0; // Active scene cannot assume there will be a related asset
    }

    void Editor::OpenSceneFromAsset(Heart::UUID uuid)
    {
        auto sceneAsset = Heart::AssetManager::RetrieveAsset<Heart::SceneAsset>(uuid);
        if (!sceneAsset || !sceneAsset->IsValid())
        {
            HE_ENGINE_LOG_ERROR("Failed to load scene");
            return;
        }
        
        OpenScene(sceneAsset->GetScene());
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
    }

    void Editor::StopScene()
    {
        s_SceneState = SceneState::Editing;
        
        s_ActiveScene->StopRuntime();
        s_ActiveScene = s_EditorScene;
    }

    bool Editor::IsDirty()
    {
        for (auto& pair : s_Windows)
            if (pair.second->IsDirty())
                return true;
        return false;
    }
}