#pragma once

namespace Heart
{
    class FilesystemUtils
    {
    public:
        static std::string ReadFileToString(const std::string& path);
        static unsigned char* ReadFile(const std::string& path, u32& outLength);
        static std::string GetParentDirectory(const std::string& path);
        static std::string SaveAsDialog(const std::string& initialPath, const std::string& title, const std::string& defaultFileName, const std::string& extension);
        static std::string OpenFileDialog(const std::string& initialPath, const std::string& title, const std::string& extension);
        static std::string OpenFolderDialog(const std::string& initialPath, const std::string& title);
    private:
        static std::string Win32OpenDialog(const std::string& initialPath, const std::string& title, const std::string& defaultFileName, const std::string& extension, bool folder, bool save);
        static std::string LinuxOpenDialog(const std::string& initialPath, const std::string& title, const std::string& defaultFileName, const std::string& extension, bool folder, bool save);
    };
}