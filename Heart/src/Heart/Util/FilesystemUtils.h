#pragma once

namespace Heart
{
    class FilesystemUtils
    {
    public:
        static std::string ReadFileToString(const std::string& path);
        static unsigned char* ReadFile(const std::string& path, u32& outLength);
        static std::string SaveAsDialog(const std::string& initialPath, const std::string& title, const std::string& defaultFileName, const std::string& extension);
        static std::string OpenFileDialog(const std::string& initialPath, const std::string& title, const std::string& extension);
        static std::string OpenFolderDialog(const std::string& initialPath, const std::string& title);
        static std::string WideToNarrowString(const std::wstring& wide);
        static std::wstring NarrowToWideString(const std::string& narrow);
    private:
        static std::string Win32OpenDialog(const std::string& initialPath, const std::string& title, const std::string& defaultFileName, const std::string& extension, bool folder, bool save);
    };
}