#pragma once

namespace MacOS
{
    struct Utils
    {
        static bool IsAppPackaged();
        static void SetCorrectWorkingDirectory();
        static std::string GetApplicationSupportDirectory();
    };
}
