#include "hepch.h"
#include "FilesystemUtils.h"

#include "Heart/Util/PlatformUtils.h"
#include "Heart/Container/HString16.h"
#include "Heart/Core/App.h"
#include "tinyfd/tinyfiledialogs.h"

#ifdef HE_PLATFORM_ANDROID
#include "Heart/Platform/Android/AndroidApp.h"
#endif

namespace Heart
{
    bool FilesystemUtils::FileExistsLocalized(const HStringView8& path)
    {
        // Checks for the existence of a file, using localized data when appropriate (i.e.
        // APK data on android). This method should be preferred when followed up with ReadFile

        #ifdef HE_PLATFORM_ANDROID
            // Ensure that the path is canonical (no ..)
            auto finalPath = std::filesystem::weakly_canonical(path.Data()).relative_path();
            AAsset* file = AAssetManager_open(AndroidApp::App->activity->assetManager, finalPath.c_str(), AASSET_MODE_BUFFER);
            bool exists = file != nullptr;
            if (exists)
                AAsset_close(file);
            return exists;
        #else
            return std::filesystem::exists(path.Data());
        #endif
    }

    void FilesystemUtils::WriteFile(const HStringView8& path, const HStringView8& data)
    {
        std::ofstream file(path.Data());
        file << data.Data();
    }

    void FilesystemUtils::WriteFile(const HStringView8& path, const nlohmann::json& data)
    {
        std::ofstream file(path.Data());
        file << data;
    }

    HString8 FilesystemUtils::ReadFileToString(const HStringView8& path)
    {
        u32 outLength = 0;
        unsigned char* data = ReadFile(path, outLength);
        if (!data)
            return HString8();
        HString8 str((char*)data, outLength);
        delete[] data;
        return str;
    }

    nlohmann::json FilesystemUtils::ReadFileToJson(const HStringView8& path)
    {
        u32 outLength = 0;
        unsigned char* data = ReadFile(path, outLength);
        if (!data)
            return nlohmann::json();
        auto j = nlohmann::json::parse(data);
        delete[] data;
        return j;
    }

    unsigned char* FilesystemUtils::ReadFile(const HStringView8& path, u32& outLength)
    {
        #ifdef HE_PLATFORM_ANDROID
            // Ensure that the path is canonical (no ..)
            auto finalPath = std::filesystem::weakly_canonical(path.Data()).relative_path();
            AAsset* file = AAssetManager_open(AndroidApp::App->activity->assetManager, finalPath.c_str(), AASSET_MODE_BUFFER);
            if (!file)
                return nullptr;

            u32 fileSize = AAsset_getLength(file);
            unsigned char* buffer = new unsigned char[fileSize + 1];
            AAsset_read(file, buffer, fileSize);
            AAsset_close(file);
        #else
            std::ifstream file(path.Data(), std::ios::ate | std::ios::binary);
            if (!file.is_open())
                return nullptr;
            
            u32 fileSize = static_cast<u32>(file.tellg());
            unsigned char* buffer = new unsigned char[fileSize + 1];
            file.seekg(0, std::ios::beg);
            file.read((char*)buffer, fileSize);
            file.close();
        #endif

        buffer[fileSize] = 0;

        outLength = fileSize;
        return buffer;
    }

    bool FilesystemUtils::DoesDirectoryExist(const HStringView8& path)
    {
        return std::filesystem::exists(path.Data());
    }

    HVector<FilesystemUtils::DirectoryEntry> FilesystemUtils::ListDirectory(const HStringView8& path)
    {
        HVector<FilesystemUtils::DirectoryEntry> result;
        
        for (const auto& entry : std::filesystem::directory_iterator(path.Data()))
            result.Add({ entry.path().filename().generic_u8string(), entry.is_directory() });

        return result;
    }

    HString8 FilesystemUtils::GetHomeDirectory()
    {
        #ifdef HE_PLATFORM_WINDOWS
            // Windows-specific: Try USERPROFILE first, then HOMEDRIVE + HOMEPATH
            const char* home = std::getenv("USERPROFILE");
            if (!home) {
                const char* homeDrive = std::getenv("HOMEDRIVE");
                const char* homePath = std::getenv("HOMEPATH");
                if (homeDrive && homePath) {
                    return std::string(homeDrive) + std::string(homePath);
                }
            }
        #else
            // Unix-like: Check the HOME environment variable
            const char* home = std::getenv("HOME");
        #endif
        return home ? HString8(home) : HString8();
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
