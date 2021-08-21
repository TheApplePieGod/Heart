#include "htpch.h"
#include "HeartEditor.h"

#include "HeartEditor/EditorLayer.h"

namespace HeartEditor
{
    HeartEditor::HeartEditor()
    {
        PushLayer(new EditorLayer());
    }
}