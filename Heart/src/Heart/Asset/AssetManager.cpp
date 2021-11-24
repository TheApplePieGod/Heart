#include "htpch.h"
#include "AssetManager.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Timing.h"

namespace Heart
{
    std::unordered_map<UUID, AssetManager::UUIDEntry> AssetManager::s_UUIDs;
    std::unordered_map<std::string, AssetManager::AssetEntry> AssetManager::s_Registry;
    std::unordered_map<std::string, AssetManager::AssetEntry> AssetManager::s_Resources;
    std::string AssetManager::s_AssetsDirectory = "D:/Projects/Heart/HeartEditor";

    void AssetManager::Initialize()
    {
        auto timer = Heart::Timer("Registering resources", false);
        RegisterAssetsInDirectory(s_ResourceDirectory, false, true);
        timer.Log();

        timer.SetName("Registering assets");
        timer.Reset();
        RegisterAssetsInDirectory(s_AssetsDirectory, false, false);
        timer.Log();
    }

    void AssetManager::Shutdown()
    {
        // cleanup assets
        for (auto& pair : s_Registry)
            pair.second.Asset.reset();

        // cleanup resources
        for (auto& pair : s_Resources)
            pair.second.Asset.reset();
    }

    void AssetManager::OnUpdate()
    {
        // check to see if assets should be unloaded
        u64 loadLimit = 1000;
        for (auto& pair : s_Registry)
        {
            if (pair.second.Persistent) continue;

            if (App::Get().GetFrameCount() > pair.second.LoadedFrame + loadLimit)
            {
                UnloadAsset(pair.second);
                HE_ENGINE_LOG_TRACE("Unloading asset @ {0}", pair.second.Asset->GetPath());
            }
        }
        for (auto& pair : s_Resources)
        {
            if (pair.second.Persistent) continue;

            if (App::Get().GetFrameCount() > pair.second.LoadedFrame + loadLimit)
            {
                UnloadAsset(pair.second);
                HE_ENGINE_LOG_TRACE("Unloading resource @ {0}", pair.second.Asset->GetPath());
            }
        }
    }

    void AssetManager::UnloadAllAssets()
    {
        for (auto& pair : s_Registry)
            UnloadAsset(pair.second);
        for (auto& pair : s_Resources)
            UnloadAsset(pair.second);
    }

    void AssetManager::LoadAllAssets()
    {
        for (auto& pair : s_Registry)
            LoadAsset(pair.second);
        for (auto& pair : s_Resources)
            LoadAsset(pair.second);
    }

    void AssetManager::LoadAsset(AssetEntry& entry)
    {
        entry.Asset->Load(); // TODO: async loading
        entry.LoadedFrame = App::Get().GetFrameCount();
    }
    
    void AssetManager::UnloadAsset(AssetEntry& entry)
    {
        entry.Asset->Unload();
        entry.LoadedFrame = std::numeric_limits<u64>::max() - s_AssetFrameLimit; // prevent extraneous unloading
    }

    UUID AssetManager::RegisterAsset(Asset::Type type, const std::string& path, bool persistent, bool isResource)
    {
        if (path.empty())
            return 0;

        UUID oldUUID = GetAssetUUID(path);
        if (oldUUID != 0)
            return oldUUID;

        UUID newUUID = UUID();
        if (isResource)
        {
            if (s_Resources.find(path) == s_Resources.end())
            {  
                HE_ENGINE_LOG_TRACE("Registering {0} resource @ {1}", HE_ENUM_TO_STRING(Asset, type), path);
                std::string absolutePath = std::filesystem::path(s_ResourceDirectory).append(path).generic_u8string();
                s_Resources[path] = { Asset::Create(type, path, absolutePath), std::numeric_limits<u64>::max() - s_AssetFrameLimit, persistent };
            }
            s_UUIDs[newUUID] = { path, isResource, type };
        }
        else
        {
            if (s_Registry.find(path) == s_Registry.end())
            {
                HE_ENGINE_LOG_TRACE("Registering {0} asset @ {1}", HE_ENUM_TO_STRING(Asset, type), path);
                std::string absolutePath = std::filesystem::path(s_AssetsDirectory).append(path).generic_u8string();
                s_Registry[path] = { Asset::Create(type, path, absolutePath), std::numeric_limits<u64>::max() - s_AssetFrameLimit, persistent };
            }
            s_UUIDs[newUUID] = { path, isResource, type };
        }

        return newUUID;    
    }

    void AssetManager::RegisterAssetsInDirectory(const std::filesystem::path& directory, bool persistent, bool isResource)
    {
        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(directory))
                if (entry.is_directory())
                    RegisterAssetsInDirectory(entry.path(), persistent, isResource);
                else
                {
                    std::string path = entry.path().generic_u8string();
                    Asset::Type type = DeduceAssetTypeFromFile(path);
                    if (type != Asset::Type::None)
                    {
                        // remove the base from the start of the path so it doesn't need to be included when referencing
                        std::string trimmedPath = path.substr((isResource ? s_ResourceDirectory.size() : s_AssetsDirectory.size()) + 1);
                        RegisterAsset(type, trimmedPath, persistent, isResource);
                    }
                }
        }
        catch (std::exception e) // likely invalid path so stop searching
        { return; }
    }

    Asset::Type AssetManager::DeduceAssetTypeFromFile(const std::string& path)
    {
        auto extension = std::filesystem::path(path).extension().generic_u8string();

        // convert the extension to lowercase
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });
        
        Asset::Type type = Asset::Type::None;
        if (extension == ".png" || extension == ".jpg" || extension == ".bmp" || extension == ".hdr") // textures
            type = Asset::Type::Texture;
        else if (extension == ".gltf")
            type = Asset::Type::Mesh;
        else if (extension == ".vert" || extension == ".frag" || extension == ".comp")
            type = Asset::Type::Shader;
        else if (extension == ".hemat")
            type = Asset::Type::Material;

        return type;
    }

    UUID AssetManager::GetAssetUUID(const std::string& path, bool isResource)
    {
        for (auto& entry : s_UUIDs)
            if (isResource == entry.second.IsResource && entry.second.Path == path) return entry.first;
        return 0;
    }

    std::string AssetManager::GetPathFromUUID(UUID uuid)
    {
        if (s_UUIDs.find(uuid) == s_UUIDs.end()) return "";
        return s_UUIDs[uuid].Path;
    }

    Asset* AssetManager::RetrieveAsset(const std::string& path, bool isResource)
    {
        if (path.empty()) return nullptr;
        if (isResource)
        { if (s_Resources.find(path) == s_Resources.end()) return nullptr; }
        else
        { if (s_Registry.find(path) == s_Registry.end()) return nullptr; }

        auto& entry = isResource ? s_Resources[path] : s_Registry[path];
        LoadAsset(entry);
        return entry.Asset.get();
    }

    Asset* AssetManager::RetrieveAsset(UUID uuid)
    {
        if (!uuid) return nullptr;
        if (s_UUIDs.find(uuid) == s_UUIDs.end()) return nullptr;

        auto& uuidEntry = s_UUIDs[uuid];
        auto& entry = uuidEntry.IsResource ? s_Resources[uuidEntry.Path] : s_Registry[uuidEntry.Path];
        LoadAsset(entry);
        return entry.Asset.get();
    }
}