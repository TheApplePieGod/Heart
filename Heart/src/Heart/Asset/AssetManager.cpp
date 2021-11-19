#include "htpch.h"
#include "AssetManager.h"

#include "Heart/Core/App.h"

namespace Heart
{
    std::unordered_map<UUID, AssetManager::UUIDEntry> AssetManager::s_UUIDs;
    std::unordered_map<std::string, AssetManager::AssetEntry> AssetManager::s_Registry;
    std::unordered_map<std::string, AssetManager::AssetEntry> AssetManager::s_Resources;
    std::string AssetManager::s_AssetsDirectory = "D:/Projects/Heart/HeartEditor";

    void AssetManager::Initialize()
    {
        
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

    bool AssetManager::IsAssetRegistered(const std::string& path)
    {
        return IsAssetRegistered(Asset::Type::None, path);
    }

    bool AssetManager::IsAssetRegistered(Asset::Type type, const std::string& path)
    {
        if (s_Registry.find(path) != s_Registry.end())
        {
            if (type == Asset::Type::None)
                return true;
            return s_Registry[path].Asset->GetType() == type;
        }

        return false;
    }

    UUID AssetManager::RegisterAsset(Asset::Type type, const std::string& path, bool persistent, bool isResource)
    {
        if (path.empty())
            return 0;

        UUID oldUUID = GetAssetUUID(path);
        if (oldUUID != 0)
            return oldUUID;

        UUID newUUID = UUID();
        s_UUIDs[newUUID] = { path, isResource };

        if (isResource)
        {
            if (s_Resources.find(path) == s_Resources.end())
            {
                HE_ENGINE_LOG_TRACE("Registering {0} resource @ {1}", HE_ENUM_TO_STRING(Asset, type), path);
                std::string absolutePath = std::filesystem::path(s_ResourcesDirectory).append(path).generic_u8string();
                s_Resources[path] = { Asset::Create(type, path, absolutePath), std::numeric_limits<u64>::max() - s_AssetFrameLimit, persistent };
            }
            return newUUID;
        }

        if (s_Registry.find(path) == s_Registry.end())
        {
            HE_ENGINE_LOG_TRACE("Registering {0} asset @ {1}", HE_ENUM_TO_STRING(Asset, type), path);
            std::string absolutePath = std::filesystem::path(s_AssetsDirectory).append(path).generic_u8string();
            s_Registry[path] = { Asset::Create(type, path, absolutePath), std::numeric_limits<u64>::max() - s_AssetFrameLimit, persistent };
        }
        return newUUID;    
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
        if (s_UUIDs.find(uuid) == s_UUIDs.end()) return nullptr;

        auto& uuidEntry = s_UUIDs[uuid];
        auto& entry = uuidEntry.IsResource ? s_Resources[uuidEntry.Path] : s_Registry[uuidEntry.Path];
        LoadAsset(entry);
        return entry.Asset.get();
    }
}