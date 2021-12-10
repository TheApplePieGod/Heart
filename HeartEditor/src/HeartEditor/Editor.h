#pragma once

#include "Heart/Scene/Entity.h"
#include "Heart/Renderer/SceneRenderer.h"

namespace HeartEditor
{
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

        static bool IsDirty();

        inline static void PushWindow(const std::string& name, const Heart::Ref<Widget>& window) { s_Windows[name] = window; }

        inline static EditorState& GetState() { return s_EditorState; }
        inline static Heart::Scene& GetActiveScene() { return *s_ActiveScene; }
        inline static void SetActiveScene(const Heart::Ref<Heart::Scene>& scene) { s_ActiveScene = scene; }
        inline static std::unordered_map<std::string, Heart::Ref<Widget>>& GetWindows() { return s_Windows; }
        inline static Widget& GetWindow(const std::string& name) { return *s_Windows[name]; }

    private:
        static EditorState s_EditorState;  
        static Heart::Ref<Heart::Scene> s_ActiveScene;
        static std::unordered_map<std::string, Heart::Ref<Widget>> s_Windows;
        static bool s_ImGuiDemoOpen;

        friend class MenuBar;
    };
}