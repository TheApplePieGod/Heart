#include "htpch.h"
#include "FilesystemUtils.h"

#include "Heart/Core/App.h"
#include "GLFW/glfw3native.h"

#ifdef HE_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace Heart
{
    std::string FilesystemUtils::ReadFileToString(const std::string& path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            return "";
        
        u32 fileSize = static_cast<u32>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), fileSize);
        file.close();

        return std::string(buffer.data(), buffer.size());
    }

    unsigned char* FilesystemUtils::ReadFile(const std::string& path, u32& outLength)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
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

    std::string FilesystemUtils::SaveAsDialog(const std::string& initialPath, const std::string& title, const std::string& defaultFileName, const std::string& extension)
    {
        #ifdef HE_PLATFORM_WINDOWS
            char szFile[_MAX_PATH] = "";
            strcpy(szFile, defaultFileName.data());

            char filter[_MAX_PATH];
            strcpy(filter, extension.data());
            filter[extension.size() + 1] = '*';
            filter[extension.size() + 2] = '.';
            strcpy(filter + extension.size() + 3, extension.data());
            filter[extension.size() * 2 + 4] = '\0';

            OPENFILENAME ofn{};
            ofn.lStructSize= sizeof(ofn);
            ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
            ofn.hInstance = GetModuleHandle(0);
            ofn.hwndOwner = glfwGetWin32Window(App::Get().GetWindow().GetWindowHandle());
            ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
            ofn.lpstrFile = szFile;
            ofn.lpstrFilter = filter;
            ofn.nFilterIndex = 1;
            ofn.lpstrDefExt = extension.c_str();
            ofn.lpstrInitialDir = initialPath.empty() ? NULL : initialPath.c_str();
            ofn.lpstrTitle = title.empty() ? NULL : title.c_str();

            if (GetSaveFileName(&ofn))
                return std::string(ofn.lpstrFile);
            else
                return "";
        #endif
    }

    std::string FilesystemUtils::OpenFileDialog(const std::string& initialPath, const std::string& title, const std::string& extension)
    {
        #ifdef HE_PLATFORM_WINDOWS
            char szFile[_MAX_PATH] = "";

            char filter[_MAX_PATH];
            strcpy(filter, extension.data());
            filter[extension.size() + 1] = '*';
            filter[extension.size() + 2] = '.';
            strcpy(filter + extension.size() + 3, extension.data());
            filter[extension.size() * 2 + 4] = '\0';

            OPENFILENAME ofn{};
            ofn.lStructSize= sizeof(ofn);
            ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
            ofn.hInstance = GetModuleHandle(0);
            ofn.hwndOwner = glfwGetWin32Window(App::Get().GetWindow().GetWindowHandle());
            ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
            ofn.lpstrFile = szFile;
            ofn.lpstrFilter = filter;
            ofn.nFilterIndex = 1;
            ofn.lpstrDefExt = extension.c_str();
            ofn.lpstrInitialDir = initialPath.empty() ? NULL : initialPath.c_str();
            ofn.lpstrTitle = title.empty() ? NULL : title.c_str();

            if (GetOpenFileName(&ofn))
                return std::string(ofn.lpstrFile);
            else
                return "";
        #endif
    }
}