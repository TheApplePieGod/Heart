#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Task/Task.h"
#include "Heart/Core/UUID.h"
#include "Heart/Container/HString8.h"

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
        /*! @brief The internal representation of an asset inside the path registry. */
        struct AssetEntry
        {
            Ref<Asset> Asset;
            bool Persistent;
            UUID Id;
        };

        /*! @brief The internal representation of an asset inside the UUID registry. */
        struct UUIDEntry
        {
            HString8 Path;
            bool IsResource;
            Asset::Type Type;
        };

    public:
        /*! @brief Initialize the manager. */
        static void Initialize();

        /*! @brief Shutdown the manager. */
        static void Shutdown();

        /*! @brief Call Unload() on all of the registered assets. */
        static void UnloadAllAssets();

        /*! @brief Call Unload() on all of the registered resources. */
        static void UnloadAllResources();

        static void EnableFileWatcher();

        static void DisableFileWatcher();

        static Task UnloadOldAssets();

        inline static bool IsInitialized() { return s_Initialized; }

        /*! @brief Get the current project directory. */
        inline static const HString8& GetAssetsDirectory() { return s_AssetsDirectory; }

        inline static const HString8& GetManifestFilename() { return s_ManifestFile; }

        inline static const HString8& GetDotDirectory() { return s_DotDir; }

        /*! @brief Get a reference to the internal asset UUID registry. */
        inline static const std::unordered_map<UUID, UUIDEntry>& GetUUIDRegistry() { return s_UUIDs; }

        inline static HString8 GetAbsolutePath(const HStringView8& relative)
        { return std::filesystem::path(s_AssetsDirectory.Data()).append(relative.Data()).generic_u8string(); }

        inline static HString8 GetRelativePath(const HStringView8& absolute)
        { return std::filesystem::path(absolute.Data()).lexically_relative(Heart::AssetManager::GetAssetsDirectory().Data()).generic_u8string(); }

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
         * @param type The type of the asset.
         * @param path The path of the asset relative to the project directory.
         * @param persistent Mark this asset as persistent (will never automatically unload).
         * @param isResource Store this asset as a resource.
         * @return The UUID of the newly registered asset.
         */
        static UUID RegisterAsset(Asset::Type type, const HString8& path, bool persistent = false, bool isResource = false);

        static void UnregisterAsset(UUID uuid);

        static UUID RegisterInMemoryAsset(Asset::Type type);

        /**
         * @brief Register all assets in a directory and all subdirectories.
         * 
         * DeduceAssetTypeFromFile() will be called for every file, and only assets
         * that match a predefined file type will be registered.
         * 
         * @param directory The absolute path of the directory to scan.
         * @param persistent Mark all assets as persistent (will never automatically unload).
         * @param isResource Store all assets as resources.
         */
        static void RegisterAssetsInDirectory(const HStringView8& directory, bool persistent = false, bool isResource = false);

        static void RegisterAssetsFromManifest(const HStringView8& directory);

        /**
         * @brief Change the name and/or path of a registered non-resource asset.
         * 
         * @note This will not reload the asset's data.
         * 
         * @param oldPath The old path of the asset relative to the project directory.
         * @param newPath The new path of the asset relative to the project directory.
         */
        static void RenameAsset(const HString8& oldPath, const HString8& newPath);

        /**
         * @brief Change the project directory.
         * 
         * This is a destructive action. It will call Unload() on all assets, remove all registered
         * assets (not resources), and call RegisterAssetsInDirectory() on the new project directory.
         * 
         * @param directory The absolute path of the new project directory.
         */
        static void UpdateAssetsDirectory(const HString8& directory);

        /**
         * @brief Get the associated asset type of a file.
         * 
         * @note The path can be either relative or absolute, as long as it contains the filename somewhere in it.
         * 
         * @param path The path of the asset.
         * @return The type of the asset or Asset::Type::None if it is unassociated.
         */
        static Asset::Type DeduceAssetTypeFromFile(const HStringView8& path);

        /**
         * @brief Get the UUID of a registered asset from its filepath.
         * 
         * @param path The filepath of the registered asset relative to the project directory.
         * @param isResource Whether or not this path references a resource asset.
         * @return The UUID of the registered path or zero if it could not be located.
         */
        static UUID GetAssetUUID(const HString8& path, bool isResource = false);

        /**
         * @brief Get the path of a registered asset from its UUID.
         * 
         * @warning There is no distinction between paths of resources and regular assets, so make
         *          sure to call IsAssetAResource() to make the distinction.
         * 
         * @param uuid The UUID of the registered asset.
         * @return The path of the asset relative to the project directory or an empty string if it could not be located.
         */
        static HString8 GetPathFromUUID(UUID uuid);

        /**
         * @brief Check if a UUID points to a resource asset.
         * 
         * @param uuid The UUID of the asset.
         * @return True if the UUID points to a resource, false if otherwise.
         */
        static bool IsAssetAResource(UUID uuid);

        /**
         * @brief Retrieve an asset from a path.
         * 
         * @param path The path of the asset relative to the project directory.
         * @param isResource Whether or not this path references a resource asset.
         * @return The pointer to the asset or nullptr if it could not be located. 
         */
        static Asset* RetrieveAsset(const HString8& path, bool isResource = false);

        /**
         * @brief Retrieve an asset from a path.
         * 
         * @tparam T The asset class to cast the pointer to upon retrieval.
         * @param path The path of the asset relative to the project directory.
         * @param isResource Whether or not this path references a resource asset.
         * @param async Whether or not to load this asset asynchronously
         * @return The pointer to the asset or nullptr if it could not be located.
         */
        template<typename T>
        inline static T* RetrieveAsset(const HString8& path, bool isResource = false)
        {
            return static_cast<T*>(RetrieveAsset(path, isResource));
        }

        /**
         * @brief Retrieve an asset from a UUID.
         * 
         * A UUID is agnostic to a resource/regular asset, which means the distinction does not need to be specified upon retrieval.
         * 
         * @param uuid The UUID of the asset.
         * @param async Whether or not to load this asset asynchronously
         * @return The pointer to the asset or nullptr if it could not be located
         */
        static Asset* RetrieveAsset(UUID uuid);

        /**
         * @brief Retrieve an asset from a UUID.
         * 
         * A UUID is agnostic to a resource/regular asset, which means the distinction does not need to be specified upon retrieval.
         * 
         * @tparam T The asset class to cast the pointer to upon retrieval.
         * @param uuid The UUID of the asset.
         * @param async Whether or not to load this asset asynchronously
         * @return The pointer to the asset or nullptr if it could not be located
         */
        template<typename T>
        inline static T* RetrieveAsset(UUID uuid)
        {
            return static_cast<T*>(RetrieveAsset(uuid));
        }

        static nlohmann::json GenerateManifest();

    private:
        static void WatchAssetDirectory();

        inline static auto& GetRegistry(bool isResource)
        { return isResource ? s_Resources : s_Registry; }

    private:
        inline static constexpr u64 s_AssetFrameLimit = 1000;
        inline static const HString8 s_ResourceDirectory = "resources";
        inline static const HString8 s_ManifestFile = "assets.json";
        inline static const HString8 s_DotDir = ".heart";
        inline static std::unordered_map<UUID, UUIDEntry> s_UUIDs;
        inline static std::unordered_map<HString8, AssetEntry> s_Registry;
        inline static std::unordered_map<HString8, AssetEntry> s_Resources;
        inline static HString8 s_AssetsDirectory;
        inline static Task s_UnloadTask;

        inline static bool s_Initialized = false;
        inline static std::shared_mutex s_Mutex;
    };
}
