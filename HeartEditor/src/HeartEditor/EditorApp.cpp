#include "hepch.h"
#include "EditorApp.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorLayer.h"
#include "Heart/Util/PerfTests.h"

namespace HeartEditor
{
    EditorApp::EditorApp()
        : App("Heart Editor")
    {
        Heart::PerfTests::RunHStringTest();
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