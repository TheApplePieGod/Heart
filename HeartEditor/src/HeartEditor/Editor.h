#pragma once

#include "Heart/Scene/Entity.h"
#include "Heart/Scene/RenderScene.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Container/HString8.h"
#include "Heart/Task/TaskManager.h"

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
        static void CreateWindows();
        static void DestroyWindows();
        static void RenderWindows();
        static void RenderWindowsPostSceneUpdate();

        static void SaveScene();
        static void OpenScene(const Heart::Ref<Heart::Scene>& scene);
        static void OpenSceneFromAsset(Heart::UUID uuid);
        static void ClearScene();
        static void PlayScene();
        static void StopScene();

        static bool IsDirty();

        inline static EditorState& GetState() { return s_EditorState; }
        inline static SceneState GetSceneState() { return s_SceneState; }
        inline static Heart::Scene& GetActiveScene() { return *s_ActiveScene; }
        inline static Heart::Scene& GetEditorScene() { return *s_EditorScene; }
        inline static Heart::RenderScene& GetRenderScene() { return s_RenderScene; }
        inline static Heart::UUID GetEditorSceneAsset() { return s_EditorSceneAsset; }
        inline static void SetEditorSceneAsset(Heart::UUID asset) { s_EditorSceneAsset = asset; }
        inline static auto& GetWindows() { return s_Windows; }
        inline static Widget& GetWindow(const Heart::HString8& name) { return *s_Windows[name]; }
        inline static const Heart::Task& GetSceneUpdateTask() { return s_SceneUpdateTask; }
        inline static void SetSceneUpdateTask(const Heart::Task& task) { s_SceneUpdateTask = task; }

        template<typename W, typename ... Args>
        static Heart::Task PushWindow(Args&& ... args)
        {
            return Heart::TaskManager::Schedule(
                [&args ...]()
                {
                    auto widget = Heart::CreateRef<W>(std::forward<Args>(args)...);
                    s_WindowsLock.lock();
                    s_Windows[widget->GetName()] = widget;
                    s_WindowsLock.unlock();
                },
                Heart::Task::Priority::High
            );
        }

    private:
        inline static EditorState s_EditorState;
        inline static Heart::Ref<Heart::Scene> s_ActiveScene, s_EditorScene;
        inline static Heart::RenderScene s_RenderScene;
        inline static Heart::Task s_SceneUpdateTask;
        inline static Heart::UUID s_EditorSceneAsset;
        inline static std::unordered_map<Heart::HString8, Heart::Ref<Widget>> s_Windows;
        inline static std::mutex s_WindowsLock;
        inline static bool s_ImGuiDemoOpen;
        inline static SceneState s_SceneState;

        friend class MenuBar;
    };
}
