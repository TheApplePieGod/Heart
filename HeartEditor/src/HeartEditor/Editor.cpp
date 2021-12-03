#include "hepch.h"
#include "Editor.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Core/Timing.h"
#include "Heart/Renderer/SceneRenderer.h"

namespace HeartEditor
{
    EditorState Editor::s_EditorState;  
    Heart::Ref<Heart::Scene> Editor::s_ActiveScene;
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

    bool Editor::IsDirty()
    {
        for (auto& window : s_Windows)
            if (window.second->IsDirty())
                return true;
        return false;
    }
}