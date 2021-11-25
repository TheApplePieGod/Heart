#pragma once

namespace Heart
{
    struct FilesystemUtils
    {
        static std::string ReadFileToString(const std::string& path);
        static unsigned char* ReadFile(const std::string& path, u32& outLength);
        static std::string SaveAsDialog(const std::string& initialPath, const std::string& title, const std::string& defaultFileName, const std::string& extension);
        static std::string OpenFileDialog(const std::string& initialPath, const std::string& title, const std::string& extension);
    };
}