#pragma once

#include "Heart/Scene/Entity.h"
#include "Heart/Renderer/SceneRenderer.h"

namespace HeartEditor
{
    enum class SceneState
    {
        Editing = 0,
        Playing
    };

    struct EditorState
    {
        Heart::Entity SelectedEntity;
        Heart::SceneRenderSettings RenderSettings;
    };

    class EditorCamera;
    class Widget;
    class Editor
    {
    public:

        static void Initialize();
        static void Shutdown();
        static void RenderWindows();

        static void OpenScene(const Heart::Ref<Heart::Scene>& scene);
        static void OpenSceneFromAsset(Heart::UUID uuid);
        static void ClearScene();
        static bool IsDirty();

        inline static void PushWindow(const std::string& name, const Heart::Ref<Widget>& window) { s_Windows[name] = window; }

        inline static EditorState& GetState() { return s_EditorState; }
        inline static SceneState GetSceneState() { return s_SceneState; }
        inline static Heart::Scene& GetActiveScene() { return *s_ActiveScene; }
        inline static Heart::Scene& GetEditorScene() { return *s_EditorScene; }
        inline static Heart::UUID GetEditorSceneAsset() { return s_EditorSceneAsset; }
        inline static std::unordered_map<std::string, Heart::Ref<Widget>>& GetWindows() { return s_Windows; }
        inline static Widget& GetWindow(const std::string& name) { return *s_Windows[name]; }

    private:
        inline static EditorState s_EditorState;
        inline static Heart::Ref<Heart::Scene> s_ActiveScene, s_EditorScene, s_RuntimeScene;
        inline static Heart::UUID s_EditorSceneAsset;
        inline static std::unordered_map<std::string, Heart::Ref<Widget>> s_Windows;
        inline static bool s_ImGuiDemoOpen;
        inline static SceneState s_SceneState;

        friend class MenuBar;
    };
}