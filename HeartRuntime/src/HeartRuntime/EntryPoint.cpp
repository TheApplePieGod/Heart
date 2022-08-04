#include "hepch.h"

#include "HeartRuntime/RuntimeApp.h"
#include "Heart/Core/Log.h"

// #ifdef HE_PLATFORM_WINDOWS
//     int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
//     {
//         return main(0, nullptr);
//     }
// #endif

int main(int argc, char** argv)
{
    Heart::Logger::Initialize();

    HeartRuntime::RuntimeApp* app = new HeartRuntime::RuntimeApp();
    app->Run();
    delete app;
    
    return 0;
}