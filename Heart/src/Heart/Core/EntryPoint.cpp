#include "htpch.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Log.h"

#ifdef HE_PLATFORM_WINDOWS

int main(int argc, char** argv)
{
    Heart::Logger::Initialize();

    Heart::App* app = new Heart::App("Heart Engine");
    app->Run();
    delete app;
    
    return 0;
}

#endif