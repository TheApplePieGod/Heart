#pragma once

namespace MacOS
{
    struct Utils
    {
        static void SetCorrectWorkingDirectory();
        static std::string GetApplicationSupportDirectory();
    };
}
