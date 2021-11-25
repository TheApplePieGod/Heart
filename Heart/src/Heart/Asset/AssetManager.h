#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"

namespace Heart
{
    class AssetManager
    {
    public:
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
            Asset::Type Type;
        };

    public:
        static void Initialize();
        static void Shutdown();
        static void UnloadAllAssets();
        static void LoadAllAssets();
        static void UnloadAllResources();
        static void LoadAllResources();
        static void OnUpdate();

        inline static const std::string& GetAssetsDirectory() { return s_AssetsDirectory; }
        inline static const std::unordered_map<UUID, UUIDEntry>& GetUUIDRegistry() { return s_UUIDs; }

        static UUID RegisterAsset(Asset::Type type, const std::string& path, bool persistent = false, bool isResource = false);
        static void RegisterAssetsInDirectory(const std::filesystem::path& directory, bool persistent = false, bool isResource = false);

        static Asset::Type DeduceAssetTypeFromFile(const std::string& path);

        static UUID GetAssetUUID(const std::string& path, bool isResource = false);
        static std::string GetPathFromUUID(UUID uuid);

        static Asset* RetrieveAsset(const std::string& path, bool isResource = false);
        template<typename T>
        static T* RetrieveAsset(const std::string& path, bool isResource = false)
        {
            return static_cast<T*>(RetrieveAsset(path, isResource));
        }

        static Asset* RetrieveAsset(UUID uuid, bool async = false);
        template<typename T>
        static T* RetrieveAsset(UUID uuid, bool async = false)
        {
            return static_cast<T*>(RetrieveAsset(uuid, async));
        }

    private:
        struct LoadOperation
        {
            bool Load;
            UUID Asset;
        };

    private:
        static void LoadAsset(AssetEntry& entry);
        static void UnloadAsset(AssetEntry& entry);
        static void ProcessQueue();
        static void PushOperation(const LoadOperation& operation);

    private:
        static inline const u64 s_AssetFrameLimit = 1000;
        static inline const std::string s_ResourceDirectory = "resources";
        static std::unordered_map<UUID, UUIDEntry> s_UUIDs;
        static std::unordered_map<std::string, AssetEntry> s_Registry;
        static std::unordered_map<std::string, AssetEntry> s_Resources;
        static std::string s_AssetsDirectory;

        static std::thread s_AssetThread;
        static std::queue<LoadOperation> s_OperationQueue;
        static std::mutex s_QueueLock;
    };
}