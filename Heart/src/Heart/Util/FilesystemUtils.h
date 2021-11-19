#pragma once

namespace Heart
{
    struct FilesystemUtils
    {
        static std::string ReadFileToString(const std::string& path);
        static unsigned char* ReadFile(const std::string& path, u32& outLength);
    };
}