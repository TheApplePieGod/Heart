#pragma once

#include "Heart/Container/HString.h"

namespace Heart
{
    class FilesystemUtils
    {
    public:
        static HString ReadFileToString(const HString& path);
        static unsigned char* ReadFile(const HString& path, u32& outLength);
        static HString GetParentDirectory(const HString& path);
        static HString SaveAsDialog(const HString& initialPath, const HString& title, const HString& defaultFileName, const HString& extension);
        static HString OpenFileDialog(const HString& initialPath, const HString& title, const HString& extension);
        static HString OpenFolderDialog(const HString& initialPath, const HString& title);
    private:
        static HString Win32OpenDialog(const HString& initialPath, const HString& title, const HString& defaultFileName, const HString& extension, bool folder, bool save);
        static HString LinuxOpenDialog(const HString& initialPath, const HString& title, const HString& defaultFileName, const HString& extension, bool folder, bool save);
    };
}