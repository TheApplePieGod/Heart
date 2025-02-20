#include "hepch.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Core/Log.h"
#include "Heart/Util/PlatformUtils.h"

int Main(int argc, char** argv)
{
    // Init platform
    Heart::PlatformUtils::InitializePlatform();

    try
    {
        Heart::Logger::Initialize("HeartEditor");
        HeartEditor::EditorApp* app = new HeartEditor::EditorApp();
        app->Run();
        delete app;
    }
    catch (std::exception& e)
    {
        HE_LOG_ERROR("Crashed: {0}", e.what());
    }
    
    return 0;
}

#ifdef HE_PLATFORM_WINDOWS
    int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
    {
        return Main(0, nullptr);
    }
#else
    int main(int argc, char** argv)
    {
        return Main(argc, argv);
    }
#endif
