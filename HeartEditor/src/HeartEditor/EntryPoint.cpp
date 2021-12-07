#include "hepch.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Core/Log.h"

int main(int argc, char** argv)
{
    Heart::Logger::Initialize();

    HeartEditor::EditorApp* app = new HeartEditor::EditorApp();
    app->Run();
    delete app;
    
    return 0;
}