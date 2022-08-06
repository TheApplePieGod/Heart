#include "hepch.h"
#include "EditorApp.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorLayer.h"

namespace HeartEditor
{
    EditorApp::EditorApp()
        : App("Heart Editor")
    {
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