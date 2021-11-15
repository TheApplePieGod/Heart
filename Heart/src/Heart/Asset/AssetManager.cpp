#include "htpch.h"
#include "AssetManager.h"

#include "Heart/Core/App.h"

namespace Heart
{
    std::unordered_map<std::string, AssetManager::AssetEntry> AssetManager::s_Registry;

    void AssetManager::Initialize()
    {
        
    }

    void AssetManager::Shutdown()
    {
        // cleanup assets
        for (auto& pair : s_Registry)
            pair.second.Asset.reset();
    }

    void AssetManager::OnUpdate()
    {
        // check to see if assets should be unloaded
        u64 loadLimit = 1000;
        for (auto& pair : s_Registry)
        {
            if (App::Get().GetFrameCount() > pair.second.LoadedFrame + loadLimit)
            {
                UnloadAsset(pair.second);
                HE_ENGINE_LOG_TRACE("Unloading asset @ {0}", pair.second.Asset->GetPath());
            }
        }
    }

    void AssetManager::UnloadAllAssets()
    {
        for (auto& pair : s_Registry)
            UnloadAsset(pair.second);
    }

    void AssetManager::LoadAllAssets()
    {
        for (auto& pair : s_Registry)
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

    Ref<Asset> AssetManager::RegisterAsset(Asset::Type type, const std::string& path)
    {
        if (s_Registry.find(path) == s_Registry.end())
        {
            HE_ENGINE_LOG_TRACE("Registering {0} asset @ {1}", HE_ENUM_TO_STRING(Asset, type), path);
            s_Registry[path] = { Asset::Create(type, path), std::numeric_limits<u64>::max() - s_AssetFrameLimit };
        }

        return s_Registry[path].Asset;
    }

    Asset* AssetManager::RetrieveAsset(const std::string& path)
    {
        HE_ENGINE_ASSERT(s_Registry.find(path) != s_Registry.end(), "Cannot retrieve asset that has not been registered");
        auto& entry = s_Registry[path];
        LoadAsset(entry);
        return entry.Asset.get();
    }
}