#include "hepch.h"
#include "FilesystemUtils.h"

#include "Heart/Util/PlatformUtils.h"
#include "Heart/Core/App.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

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

    std::string FilesystemUtils::GetParentDirectory(const std::string& path)
    {
        return std::filesystem::path(path).parent_path().generic_u8string();
    }

    std::string FilesystemUtils::SaveAsDialog(const std::string& initialPath, const std::string& title, const std::string& defaultFileName, const std::string& extension)
    {
        #ifdef HE_PLATFORM_WINDOWS
            return Win32OpenDialog(initialPath, title, defaultFileName, extension, false, true);
        #elif defined(HE_PLATFORM_LINUX)
            return LinuxOpenDialog(initialPath, title, defaultFileName, extension, false, true);
        #endif

        return "";
    }

    std::string FilesystemUtils::OpenFileDialog(const std::string& initialPath, const std::string& title, const std::string& extension)
    {
        #ifdef HE_PLATFORM_WINDOWS
            return Win32OpenDialog(initialPath, title, "", extension, false, false);
        #elif defined(HE_PLATFORM_LINUX)
            return LinuxOpenDialog(initialPath, title, "", extension, false, false);
        #endif
        
        return "";
    }

    // https://cpp.hotexamples.com/examples/-/IFileDialog/-/cpp-ifiledialog-class-examples.html
    std::string FilesystemUtils::OpenFolderDialog(const std::string& initialPath, const std::string& title)
    {
        #ifdef HE_PLATFORM_WINDOWS
            return Win32OpenDialog(initialPath, title, "", "", true, false);
        #elif defined(HE_PLATFORM_LINUX)
            return LinuxOpenDialog(initialPath, title, "", "", true, false);
        #endif

        return "";
    }

    std::string FilesystemUtils::Win32OpenDialog(const std::string& initialPath, const std::string& title, const std::string& defaultFileName, const std::string& extension, bool folder, bool save)
    {
        HE_ENGINE_ASSERT(!folder || !save, "Cannot open a dialog with folder and save flags set to true");

        std::string outputPath = "";
        #ifdef HE_PLATFORM_WINDOWS
            HRESULT hr = S_OK;

            IFileDialog* pDialog = nullptr;
            IShellItem* pItem = nullptr;
            LPWSTR pwszFilePath = NULL;
            bool success = false;

            // Create the FileOpenDialog object.
            hr = CoCreateInstance(
                save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, 
                NULL, 
                CLSCTX_INPROC_SERVER, 
                IID_PPV_ARGS(&pDialog)
            );
            if (FAILED(hr)) goto done;

            if (!title.empty())
            {
                hr = pDialog->SetTitle(PlatformUtils::NarrowToWideString(title).c_str());
                if (FAILED(hr)) goto done;
            }

            DWORD dwOptions;
            hr = pDialog->GetOptions(&dwOptions);
            if (FAILED(hr)) goto done;
            pDialog->SetOptions(dwOptions | (folder ? FOS_PICKFOLDERS : 0) | (save ? FOS_PATHMUSTEXIST : 0) | FOS_NOCHANGEDIR | FOS_OVERWRITEPROMPT);

            if (!folder)
            {
                std::wstring wideExtension = PlatformUtils::NarrowToWideString(extension);
                std::wstring filterFirst = wideExtension + L" (*." + wideExtension + L")";
                std::wstring filterSecond = L"*." + wideExtension;
                COMDLG_FILTERSPEC rgSpec[] = 
                {
                    { L"All Files (*.*)", L"*.*" },
                    { filterFirst.c_str(), filterSecond.c_str() }
                };
                hr = pDialog->SetFileTypes(extension.empty() ? 1 : ARRAYSIZE(rgSpec), rgSpec);
                if (FAILED(hr)) goto done;

                pDialog->SetFileTypeIndex(2);
                if (save)
                    pDialog->SetDefaultExtension(wideExtension.c_str());

                if (save && !defaultFileName.empty())
                {
                    hr = pDialog->SetFileName(PlatformUtils::NarrowToWideString(defaultFileName).c_str());
                    if (FAILED(hr)) goto done;
                }
            }

            hr = pDialog->Show(NULL);
            if (FAILED(hr) || hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) goto done;

            hr = pDialog->GetResult(&pItem);
            if (FAILED(hr)) goto done;

            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwszFilePath);
            if (FAILED(hr)) goto done;

            outputPath = PlatformUtils::WideToNarrowString(pwszFilePath);

            success = true;

        done:
            if (!success && hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
            {
                _com_error err(hr);
                LPCTSTR errMsg = err.ErrorMessage();
                HE_ENGINE_LOG_ERROR("Failed to open file dialog: {0}", errMsg);
            }
            if (pDialog)
                pDialog->Release();
            if (pItem)
                pItem->Release();
        #endif
        return outputPath;
    }

    std::string FilesystemUtils::LinuxOpenDialog(const std::string& initialPath, const std::string& title, const std::string& defaultFileName, const std::string& extension, bool folder, bool save)
    {
        HE_ENGINE_ASSERT(!folder || !save, "Cannot open a dialog with folder and save flags set to true");

        std::string outputPath = "";
        #ifdef HE_PLATFORM_LINUX
            const char zenityPath[] = "/usr/bin/zenity";
            char command[2048];

            if (folder)
            {
                sprintf(
                    command,
                    "%s --file-selection --directory --modal --confirm-overwrite --title=\"%s\" ",
                    zenityPath,
                    title.c_str()
                );
            }
            else
            {
                sprintf(
                    command,
                    "%s --file-selection %s --filename=\"%s.%s\" --file-filter=\"%s | *.%s\" --file-filter=\"All files | *.*\" --modal --confirm-overwrite --title=\"%s\" ",
                    zenityPath,
                    save ? "--save" : "",
                    defaultFileName.c_str(),
                    extension.c_str(),
                    extension.c_str(),
                    extension.c_str(),
                    title.c_str()
                );
            }

            char filename[1024];
            filename[0] = 0;

            FILE* f = popen(command, "r");
            if (!f)
                return outputPath;
            fgets(filename, 1024, f);
            pclose(f);

            outputPath = filename;
        #endif
        return outputPath;
    }
}