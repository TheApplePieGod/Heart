#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"

namespace Heart
{
    class AssetManager
    {
    public:
        static void Initialize();
        static void Shutdown();
        static void UnloadAllAssets();
        static void LoadAllAssets();
        static void UnloadAllResources();
        static void LoadAllResources();
        static void OnUpdate();

        static const std::string& GetAssetsDirectory() { return s_AssetsDirectory; }
        static bool IsAssetRegistered(const std::string& path);
        static bool IsAssetRegistered(Asset::Type type, const std::string& path);
        static UUID RegisterAsset(Asset::Type type, const std::string& path, bool persistent = false, bool isResource = false);

        static UUID GetAssetUUID(const std::string& path, bool isResource = false);
        static std::string GetPathFromUUID(UUID uuid);

        static Asset* RetrieveAsset(const std::string& path, bool isResource = false);
        template<typename T>
        static T* RetrieveAsset(const std::string& path, bool isResource = false)
        {
            return static_cast<T*>(RetrieveAsset(path, isResource));
        }

        static Asset* RetrieveAsset(UUID uuid);
        template<typename T>
        static T* RetrieveAsset(UUID uuid)
        {
            return static_cast<T*>(RetrieveAsset(uuid));
        }

    private:
        struct AssetEntry
        {
            Ref<Asset> Asset;
            u64 LoadedFrame;
            bool Persistent;
        };
        struct UUIDEntry
        {
            std::string Path;
            bool IsResource;
        };

    private:
        static void LoadAsset(AssetEntry& entry);
        static void UnloadAsset(AssetEntry& entry);

    private:
        static inline const u64 s_AssetFrameLimit = 1000;
        static inline const std::string s_ResourcesDirectory = "resources"; 
        static std::unordered_map<UUID, UUIDEntry> s_UUIDs;
        static std::unordered_map<std::string, AssetEntry> s_Registry;
        static std::unordered_map<std::string, AssetEntry> s_Resources;
        static std::string s_AssetsDirectory;
    };
}