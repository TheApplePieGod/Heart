#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"

namespace Heart
{
    class AssetManager
    {
    public:
        static void Initialize();
        static void ReloadMetadata();
        static void SaveMetadata();
        static void Shutdown();
        static void UnloadAllAssets();
        static void LoadAllAssets();
        static void OnUpdate();

        static bool IsAssetRegistered(const std::string& path);
        static bool IsAssetRegistered(Asset::Type type, const std::string& path);
        static Ref<Asset> RegisterAsset(Asset::Type type, const std::string& path);    
        static Asset* RetrieveAsset(const std::string& path);
        template<typename T>
        static T* RetrieveAsset(const std::string& path)
        {
            return static_cast<T*>(RetrieveAsset(path));
        }

    private:
        struct AssetEntry
        {
            Ref<Asset> Asset;
            u64 LoadedFrame;
        };

    private:
        static void LoadAsset(AssetEntry& entry);
        static void UnloadAsset(AssetEntry& entry);

    private:
        static inline const u64 s_AssetFrameLimit = 1000; 
        static std::unordered_map<std::string, AssetEntry> s_Registry;
    };
}