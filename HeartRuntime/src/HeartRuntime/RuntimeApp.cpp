#include "hepch.h"
#include "RuntimeApp.h"

#include "HeartRuntime/RuntimeLayer.h"

namespace HeartRuntime
{
    RuntimeApp::RuntimeApp(const std::filesystem::path& projectPath)
        : App(projectPath.stem().generic_u8string())
    {
        PushLayer(Heart::CreateRef<RuntimeLayer>(projectPath));
    }
}