#include "hepch.h"
#include "FilesystemUtils.h"

#include "Heart/Util/PlatformUtils.h"
#include "Heart/Container/HString16.h"
#include "Heart/Core/App.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "tinyfd/tinyfiledialogs.h"

namespace Heart
{
    HString8 FilesystemUtils::ReadFileToString(const HStringView8& path)
    {
        std::ifstream file(path.Data(), std::ios::ate | std::ios::binary);
        if (!file.is_open())
            return "";
        
        u32 fileSize = static_cast<u32>(file.tellg());
        HVector<char> buffer(fileSize);
        file.seekg(0, std::ios::beg);
        file.read(buffer.Data(), fileSize);
        file.close();

        return HString8(buffer.Data(), buffer.Count());
    }

    unsigned char* FilesystemUtils::ReadFile(const HStringView8& path, u32& outLength)
    {
        std::ifstream file(path.Data(), std::ios::ate | std::ios::binary);
        if (!file.is_open())
            return nullptr;
        
        u32 fileSize = static_cast<u32>(file.tellg());
        unsigned char* buffer = new unsigned char[fileSize + 1];
        file.seekg(0, std::ios::beg);
        file.read((char*)buffer, fileSize);
        buffer[fileSize] = 0;
        file.close();

        outLength = fileSize;
        return buffer;
    }

    HString8 FilesystemUtils::GetParentDirectory(const HStringView8& path)
    {
        return std::filesystem::path(path.Data()).parent_path().generic_u8string();
    }

    HString8 FilesystemUtils::SaveAsDialog(const HStringView8& initialPath, const HStringView8& title, const HStringView8& defaultFileName, const HStringView8& _filter)
    {
        const char* filter = _filter.Data();
        return tinyfd_saveFileDialog(title.Data(), initialPath.Data(), 1, &filter, nullptr);
    }

    HString8 FilesystemUtils::OpenFileDialog(const HStringView8& initialPath, const HStringView8& title, const HStringView8& _filter)
    {
        const char* filter = _filter.Data();
        return tinyfd_openFileDialog(title.Data(), initialPath.Data(), 1, &filter, nullptr, 0);
    }

    HString8 FilesystemUtils::OpenFolderDialog(const HStringView8& initialPath, const HStringView8& title)
    {
        return tinyfd_selectFolderDialog(title.Data(), initialPath.Data());
    }
}