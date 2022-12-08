#include "hepch.h"
#include "FilesystemUtils.h"

#include "Heart/Util/PlatformUtils.h"
#include "Heart/Container/HString16.h"
#include "Heart/Core/App.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

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

    HString8 FilesystemUtils::SaveAsDialog(const HStringView8& initialPath, const HStringView8& title, const HStringView8& defaultFileName, const HStringView8& extension)
    {
        #ifdef HE_PLATFORM_WINDOWS
            return Win32OpenDialog(initialPath, title, defaultFileName, extension, false, true);
        #elif defined(HE_PLATFORM_LINUX)
            return LinuxOpenDialog(initialPath, title, defaultFileName, extension, false, true);
        #endif

        return "";
    }

    HString8 FilesystemUtils::OpenFileDialog(const HStringView8& initialPath, const HStringView8& title, const HStringView8& extension)
    {
        #ifdef HE_PLATFORM_WINDOWS
            return Win32OpenDialog(initialPath, title, "", extension, false, false);
        #elif defined(HE_PLATFORM_LINUX)
            return LinuxOpenDialog(initialPath, title, "", extension, false, false);
        #endif
        
        return "";
    }

    // https://cpp.hotexamples.com/examples/-/IFileDialog/-/cpp-ifiledialog-class-examples.html
    HString8 FilesystemUtils::OpenFolderDialog(const HStringView8& initialPath, const HStringView8& title)
    {
        #ifdef HE_PLATFORM_WINDOWS
            return Win32OpenDialog(initialPath, title, "", "", true, false);
        #elif defined(HE_PLATFORM_LINUX)
            return LinuxOpenDialog(initialPath, title, "", "", true, false);
        #endif

        return "";
    }

    HString8 FilesystemUtils::Win32OpenDialog(const HStringView8& initialPath, const HStringView8& title, const HStringView8& defaultFileName, const HStringView8& extension, bool folder, bool save)
    {
        HE_ENGINE_ASSERT(!folder || !save, "Cannot open a dialog with folder and save flags set to true");

        HString8 outputPath;
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

            if (!title.IsEmpty())
            {
                hr = pDialog->SetTitle((LPCWSTR)title.ToUTF16().Data());
                if (FAILED(hr)) goto done;
            }

            DWORD dwOptions;
            hr = pDialog->GetOptions(&dwOptions);
            if (FAILED(hr)) goto done;
            pDialog->SetOptions(dwOptions | (folder ? FOS_PICKFOLDERS : 0) | (save ? FOS_PATHMUSTEXIST : 0) | FOS_NOCHANGEDIR | FOS_OVERWRITEPROMPT);

            if (!folder)
            {
                HString16 wideExtension = extension.ToUTF16();
                HString16 filterFirst = wideExtension + u" (*." + wideExtension + u")";
                HString16 filterSecond = HString16(u"*.") + wideExtension;
                COMDLG_FILTERSPEC rgSpec[] = 
                {
                    { L"All Files (*.*)", L"*.*" },
                    { (LPCWSTR)filterFirst.Data(), (LPCWSTR)filterSecond.Data() }
                };
                hr = pDialog->SetFileTypes(extension.IsEmpty() ? 1 : ARRAYSIZE(rgSpec), rgSpec);
                if (FAILED(hr)) goto done;

                pDialog->SetFileTypeIndex(2);
                if (save)
                    pDialog->SetDefaultExtension((LPCWSTR)wideExtension.Data());

                if (save && !defaultFileName.IsEmpty())
                {
                    hr = pDialog->SetFileName((LPCWSTR)defaultFileName.ToUTF16().Data());
                    if (FAILED(hr)) goto done;
                }
            }

            hr = pDialog->Show(NULL);
            if (FAILED(hr) || hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) goto done;

            hr = pDialog->GetResult(&pItem);
            if (FAILED(hr)) goto done;

            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwszFilePath);
            if (FAILED(hr)) goto done;

            outputPath = HString16((char16*)pwszFilePath).ToUTF8();
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

    HString8 FilesystemUtils::LinuxOpenDialog(const HStringView8& initialPath, const HStringView8& title, const HStringView8& defaultFileName, const HStringView8& extension, bool folder, bool save)
    {
        HE_ENGINE_ASSERT(!folder || !save, "Cannot open a dialog with folder and save flags set to true");

        HString8 outputPath;
        #ifdef HE_PLATFORM_LINUX
            const char zenityPath[] = "/usr/bin/zenity";
            char command[2048];

            if (folder)
            {
                sprintf(
                    command,
                    "%s --file-selection --directory --modal --confirm-overwrite --title=\"%s\" ",
                    zenityPath,
                    title.Data()
                );
            }
            else
            {
                sprintf(
                    command,
                    "%s --file-selection %s --filename=\"%s.%s\" --file-filter=\"%s | *.%s\" --file-filter=\"All files | *.*\" --modal --confirm-overwrite --title=\"%s\" ",
                    zenityPath,
                    save ? "--save" : "",
                    defaultFileName.Data(),
                    extension.Data(),
                    extension.Data(),
                    extension.Data(),
                    title.Data()
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