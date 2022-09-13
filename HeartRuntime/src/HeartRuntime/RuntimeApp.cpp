#include "hepch.h"
#include "RuntimeApp.h"

#include "HeartRuntime/RuntimeLayer.h"

namespace HeartRuntime
{
    RuntimeApp::RuntimeApp()
        : App("Heart Runtime")
    {
        PushLayer(Heart::CreateRef<RuntimeLayer>());
    }
}