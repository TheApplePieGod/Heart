#include "hepch.h"
#include "RuntimeApp.h"

#include "HeartRuntime/RuntimeLayer.h"
#include "Heart/Core/Window.h"

namespace HeartRuntime
{
    RuntimeApp::RuntimeApp(const std::filesystem::path& projectPath)
        : App()
    {
        // TODO: fix title
        Heart::WindowCreateInfo mainWindow;
        mainWindow.Title = "Heart Game";
        mainWindow.Width = 1920;
        mainWindow.Height = 1080;
        OpenWindow(mainWindow);

        PushLayer(Heart::CreateRef<RuntimeLayer>(projectPath));
    }
}
