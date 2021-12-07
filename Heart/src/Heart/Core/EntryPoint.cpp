#include "hepch.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Log.h"

int main(int argc, char** argv)
{
    Heart::Logger::Initialize();

    Heart::App* app = new Heart::App("Heart Engine");
    app->Run();
    delete app;
    
    return 0;
}