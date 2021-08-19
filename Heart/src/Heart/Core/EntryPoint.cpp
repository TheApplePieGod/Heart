#include "htpch.h"

#include "Heart/Core/Engine.h"
#include "Heart/Core/Log.h"

#ifdef HE_PLATFORM_WINDOWS

int main(int argc, char** argv)
{
    Heart::Logger::Initialize();

    Heart::Engine* engine = new Heart::Engine();
    engine->Run();
    delete engine;
    
    return 0;
}

#endif