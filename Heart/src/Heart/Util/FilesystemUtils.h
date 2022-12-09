#pragma once

#include "Heart/Container/HString8.h"

namespace Heart
{
    class FilesystemUtils
    {
    public:
        static HString8 ReadFileToString(const HStringView8& path);
        static unsigned char* ReadFile(const HStringView8& path, u32& outLength);
        static HString8 GetParentDirectory(const HStringView8& path);
        static HString8 SaveAsDialog(const HStringView8& initialPath, const HStringView8& title, const HStringView8& defaultFileName, const HStringView8& filter);
        static HString8 OpenFileDialog(const HStringView8& initialPath, const HStringView8& title, const HStringView8& filter);
        static HString8 OpenFolderDialog(const HStringView8& initialPath, const HStringView8& title);
    };
}