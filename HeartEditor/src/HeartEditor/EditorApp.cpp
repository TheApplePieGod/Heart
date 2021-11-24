#include "htpch.h"
#include "EditorApp.h"

#include "HeartEditor/EditorLayer.h"

namespace HeartEditor
{
    EditorApp::EditorApp()
        : App("Heart Editor")
    {
        PushLayer(new EditorLayer());
    }

    void EditorApp::Close()
    {
        EditorLayer* editorLayer = (EditorLayer*)m_Layers[0];
        if (editorLayer->IsDirty())
            App::CloseWithConfirmation();  
        else
            App::Close();
    }
}