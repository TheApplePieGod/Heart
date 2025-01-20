#include "hepch.h"

#include "HeartRuntime/RuntimeApp.h"
#include "Heart/Core/Log.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/PlatformUtils.h"
#include "Heart/Util/FilesystemUtils.h"

#ifdef HE_PLATFORM_ANDROID
#include "Heart/Platform/Android/AndroidApp.h"
#endif

int Main(int argc, char** argv)
{
    try
    {
        Heart::PlatformUtils::InitializePlatform();

        nlohmann::json metadata = Heart::FilesystemUtils::ReadFileToJson("metadata.json");
        if (metadata.is_null())
            return 0;

        // Init
        Heart::HString8 projectName = metadata["projectName"];
        Heart::Logger::Initialize(projectName.Data());
        Heart::AssetManager::UpdateAssetsDirectory("project");

        auto projectPath = std::filesystem::path("project")
            .append((projectName + ".heproj").Data());

        HeartRuntime::RuntimeApp* app = new HeartRuntime::RuntimeApp(projectPath);
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
#elif defined(HE_PLATFORM_ANDROID)
    static void AndroidAppHandleCmd(struct android_app *app, int cmd)
    {
        switch (cmd) {
            case APP_CMD_START: {
                break;
            }
            case APP_CMD_RESUME: {
                Heart::AndroidApp::Paused = false;
                break;
            }
            case APP_CMD_PAUSE: {
                Heart::AndroidApp::Paused = true;
                break;
            }
            case APP_CMD_STOP: {
                break;
            }
            case APP_CMD_DESTROY: {
                Heart::AndroidApp::NativeWindow = nullptr;
                break;
            }
            case APP_CMD_INIT_WINDOW: {
                Heart::AndroidApp::NativeWindow = app->window;
                break;
            }
            case APP_CMD_TERM_WINDOW: {
                Heart::AndroidApp::NativeWindow = nullptr;
                break;
            }
        }
    }

    // https://github.com/KhronosGroup/OpenXR-Tutorials/blob/main/Chapter1/main.cpp
    void android_main(struct android_app* app)
    {
        Heart::AndroidApp::App = app;
        app->onAppCmd = AndroidAppHandleCmd;
        Main(0, nullptr);
    }
#else
    int main(int argc, char** argv)
    {
        return Main(argc, argv);
    }
#endif
