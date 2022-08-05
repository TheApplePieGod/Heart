#pragma once

#include "Heart/Container/HString.h"

namespace Heart
{
    class FilesystemUtils
    {
    public:
        static HString ReadFileToString(const HStringView& path);
        static unsigned char* ReadFile(const HStringView& path, u32& outLength);
        static HString GetParentDirectory(const HStringView& path);
        static HString SaveAsDialog(const HStringView& initialPath, const HStringView& title, const HStringView& defaultFileName, const HStringView& extension);
        static HString OpenFileDialog(const HStringView& initialPath, const HStringView& title, const HStringView& extension);
        static HString OpenFolderDialog(const HStringView& initialPath, const HStringView& title);
    private:
        static HString Win32OpenDialog(const HStringView& initialPath, const HStringView& title, const HStringView& defaultFileName, const HStringView& extension, bool folder, bool save);
        static HString LinuxOpenDialog(const HStringView& initialPath, const HStringView& title, const HStringView& defaultFileName, const HStringView& extension, bool folder, bool save);
    };
}