#include "hepch.h"

#include "HeartRuntime/RuntimeApp.h"
#include "Heart/Core/Log.h"
#include "Heart/Util/PlatformUtils.h"

int Main(int argc, char** argv)
{
    // Init platform
    Heart::PlatformUtils::InitializePlatform();

    // Locate project folder
    auto projectFolderPath = std::filesystem::path("project");
    if (!std::filesystem::exists(projectFolderPath))
    {
        HE_LOG_ERROR("Unable to locate project folder");
        throw std::exception();
    }

    // Locate project
    auto projectPath = std::filesystem::path();
    for (const auto& entry : std::filesystem::directory_iterator(projectFolderPath))
    {
        if (!entry.is_directory() && entry.path().extension() == ".heproj")
        {
            projectPath = entry.path();
            break;
        }
    }
    if (projectPath.empty())
    {
        HE_LOG_ERROR("Unable to locate project file");
        throw std::exception();
    }

    HeartRuntime::RuntimeApp* app = new HeartRuntime::RuntimeApp(projectPath);
    app->Run();
    delete app;
    
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