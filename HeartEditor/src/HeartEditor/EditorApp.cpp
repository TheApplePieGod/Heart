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
}