#include "hepch.h"
#include "EditorApp.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorLayer.h"
#include "HeartEditor/StartupLayer.h"
#include "Heart/ImGui/ImGuiInstance.h"

namespace HeartEditor
{
    EditorApp::EditorApp()
        : App()
    {
        // Init editor
        Editor::Initialize();

        PushLayer(Heart::CreateRef<StartupLayer>());
    }

    EditorApp::~EditorApp()
    {
        Editor::Shutdown();
    }

    void EditorApp::StartEditor(Heart::HStringView8 windowName)
    {
        PopLayer();

        PushLayer(Heart::CreateRef<EditorLayer>(windowName));
    }

    void EditorApp::Close()
    {
        if (Editor::IsDirty())
            App::CloseWithConfirmation();  
        else
            App::Close();
    }
}
