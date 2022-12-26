#include "hepch.h"
#include "EditorApp.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorLayer.h"
#include "Heart/ImGui/ImGuiInstance.h"

namespace HeartEditor
{
    EditorApp::EditorApp()
        : App("Heart Editor")
    {
        // Load default ini file (will remove this when no-project editor support gets removed)
        m_ImGuiInstance->OverrideImGuiConfig("imgui.ini");
        m_ImGuiInstance->ReloadImGuiConfig();

        PushLayer(Heart::CreateRef<EditorLayer>());
    }

    void EditorApp::Close()
    {
        if (Editor::IsDirty())
            App::CloseWithConfirmation();  
        else
            App::Close();
    }
}