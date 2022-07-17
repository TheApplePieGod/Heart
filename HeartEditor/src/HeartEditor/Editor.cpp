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
    EditorState Editor::s_EditorState;  
    Heart::Ref<Heart::Scene> Editor::s_ActiveScene;
    Heart::UUID Editor::s_ActiveSceneAsset;
    std::unordered_map<std::string, Heart::Ref<Widget>> Editor::s_Windows;
    bool Editor::s_ImGuiDemoOpen = false;

    void Editor::Initialize()
    {
        s_ActiveScene = Heart::CreateRef<Heart::Scene>();
    }

    void Editor::Shutdown()
    {
        s_Windows.clear();
        s_ActiveScene.reset();
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

    void Editor::SetActiveScene(const Heart::Ref<Heart::Scene>& scene)
    {
        s_ActiveScene = scene;
        s_EditorState.SelectedEntity = Heart::Entity();
        s_ActiveSceneAsset = 0; // Active scene cannot assume there will be a related asset
    }

    void Editor::SetActiveSceneFromAsset(Heart::UUID uuid)
    {
        auto sceneAsset = Heart::AssetManager::RetrieveAsset<Heart::SceneAsset>(uuid);
        if (!sceneAsset || !sceneAsset->IsValid())
        {
            HE_ENGINE_LOG_ERROR("Failed to load scene");
            return;
        }
        
        SetActiveScene(sceneAsset->GetScene());
        s_ActiveSceneAsset = uuid;
    }

    bool Editor::IsDirty()
    {
        for (auto& pair : s_Windows)
            if (pair.second->IsDirty())
                return true;
        return false;
    }
}