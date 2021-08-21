#include "htpch.h"

#include "HeartEditor/HeartEditor.h"
#include "Heart/Core/Log.h"


#ifdef HE_PLATFORM_WINDOWS

int main(int argc, char** argv)
{
    Heart::Logger::Initialize();

    HeartEditor::HeartEditor* app = new HeartEditor::HeartEditor();
    app->Run();
    delete app;
    
    return 0;
}

#endif