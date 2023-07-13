#pragma once

#include "Heart/Core/App.h"

namespace HeartRuntime
{
    class RuntimeApp : public Heart::App
    {
    public:
        RuntimeApp(const std::filesystem::path& projectPath);
        ~RuntimeApp() = default;

    private:

    };
}