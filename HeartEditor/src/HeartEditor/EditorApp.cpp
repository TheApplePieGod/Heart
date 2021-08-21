#include "htpch.h"
#include "EditorApp.h"

#include "HeartEditor/EditorLayer.h"

namespace HeartEditor
{
    EditorApp::EditorApp()
    {
        PushLayer(new EditorLayer());
    }
}