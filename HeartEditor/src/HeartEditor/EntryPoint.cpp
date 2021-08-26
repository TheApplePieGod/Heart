#include "htpch.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Core/Log.h"


#ifdef HE_PLATFORM_WINDOWS

int main(int argc, char** argv)
{
    Heart::Logger::Initialize();

    HeartEditor::EditorApp* app = new HeartEditor::EditorApp();
    app->Run();
    delete app;
    
    return 0;
}

#endif