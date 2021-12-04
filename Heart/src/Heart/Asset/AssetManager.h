#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"

namespace Heart
{
    /**
     * @brief The static manager of all assets in the engine.
     * 
     * The manager will automatically handle the lifecycle of assets and their data
     * while providing a simple interface for retrieving them. The manager stores two different
     * types of assets: resources and regular assets. A resource is still an asset, but it is
     * one that explicitly builds with the engine and therefore is assumed to always exist.
     * A resource always lives in the {s_ResourceDirectory} directory in the executable path.
     * A regular asset, however, is not assumed to exist and lives rooted under the {s_AssetsDirectory}
     * directory, which can be modified at runtime.
     */
    class AssetManager
    {
    public:
        /**
         * @brief 
         * 
         */
        struct AssetEntry
        {
            Ref<Asset> Asset;
            u64 LoadedFrame;
            bool Persistent;
        };

        /**
         * @brief 
         * 
         */
        struct UUIDEntry
        {
            std::string Path;
            bool IsResource;
            Asset::Type Type;
        };

    public:
        /*! @brief Initialize the manager */
        static void Initialize();

        /*! @brief Shutdown the manager */
        static void Shutdown();

        /*! @brief Call Unload() on all of the registered assets */
        static void UnloadAllAssets();

        /*! @brief Call Load() on all of the registered assets */
        static void LoadAllAssets();

        /*! @brief Initialize the manager */
        static void UnloadAllResources();
        static void LoadAllResources();
        static void OnUpdate();

        inline static const std::string& GetAssetsDirectory() { return s_AssetsDirectory; }
        inline static const std::unordered_map<UUID, UUIDEntry>& GetUUIDRegistry() { return s_UUIDs; }
        inline static std::string GetAbsolutePath(const std::string& relative) { return std::filesystem::path(s_AssetsDirectory).append(relative).generic_u8string(); }

        /**
         * @brief Register an asset in the manager's registry.
         * 
         * Because these resources and regular assets are stored independently, it is impossible
         * to have naming collisions. For example, registering a resource at 'folder/Test.png' will
         * not prevent you from registering a regular asset with the same path because they are distinguished
         * by the isResource flag.
         * 
         * @note Registering the asset will not load its data.
         * 
         * @param type The type of the asset
         * @param path The path of the asset relative to the project directory.
         * @param persistent Mark this asset as persistent (will never automatically unload).
         * @param isResource Store this asset as a resource.
         * @return The UUID of the newly registered asset.
         */
        static UUID RegisterAsset(Asset::Type type, const std::string& path, bool persistent = false, bool isResource = false);

        /**
         * @brief Register all assets in a directory and all subdirectories
         * 
         * DeduceAssetTypeFromFile() will be called for every file
         * 
         * @param directory The absolute path of the directory to scan
         * @param persistent Mark all assets as persistent (will never automatically unload).
         * @param isResource Store all assets as resources.
         */
        static void RegisterAssetsInDirectory(const std::filesystem::path& directory, bool persistent = false, bool isResource = false);

        static void RenameAsset(const std::string& oldPath, const std::string& newPath);
        static void UpdateAssetsDirectory(const std::string& directory);

        static Asset::Type DeduceAssetTypeFromFile(const std::string& path);

        static UUID GetAssetUUID(const std::string& path, bool isResource = false);
        static std::string GetPathFromUUID(UUID uuid);
        static bool IsAssetAnEngineResource(UUID uuid);

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