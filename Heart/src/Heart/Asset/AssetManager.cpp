#include "hepch.h"
#include "AssetManager.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Timing.h"
#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Task/TaskManager.h"
#include "efsw/efsw.hpp"

namespace Heart
{
#ifndef HE_PLATFORM_ANDROID
    class UpdateListener : public efsw::FileWatchListener
    {
    public:
        void handleFileAction(
            efsw::WatchID watchid,
            const std::string& dir,
            const std::string& filename,
            efsw::Action action,
            std::string oldFilename
        ) override
        {
            auto newPath = std::filesystem::path(dir).append(filename);
            switch (action)
            {
                case efsw::Actions::Add:
                    m_FileAddedCallback(dir, filename);
                    break;
                case efsw::Actions::Delete:
                    m_FileDeletedCallback(dir, filename);
                    break;
                case efsw::Actions::Modified:
                    m_FileModifiedCallback(dir, filename);
                    break;
                case efsw::Actions::Moved:
                    m_FileRenamedCallback(dir, oldFilename, filename);
                    break;
                default:
                    break;
            }
        }

    private:
        std::function<void(HStringView8, HStringView8)> m_FileDeletedCallback;
        std::function<void(HStringView8, HStringView8)> m_FileAddedCallback;
        std::function<void(HStringView8, HStringView8)> m_FileModifiedCallback;
        std::function<void(HStringView8, HStringView8, HStringView8)> m_FileRenamedCallback;
    };

    // esfw static variables
    static int s_WatchId = 0;
    static Scope<UpdateListener> s_UpdateListener = nullptr;
    static Scope<efsw::FileWatcher> s_FileWatcher = nullptr;
#endif

    void AssetManager::Initialize()
    {
        #ifdef HE_DIST
            // Register from manifest if assets dir is set
            if (!s_AssetsDirectory.IsEmpty())
            {
                auto timer = Heart::Timer("Registering asset manifest", false);
                RegisterAssetsFromManifest(s_AssetsDirectory);
                timer.Log();
            }
            else
                HE_ENGINE_LOG_INFO("Assets directory not specified, skipping manifest registration");
        #else
            // Register from directories
            auto timer = Heart::Timer("Registering resources", false);
            RegisterAssetsInDirectory(s_ResourceDirectory, false, true);
            timer.Log();

            if (!s_AssetsDirectory.IsEmpty())
            {
                timer.SetName("Registering assets");
                timer.Reset();
                RegisterAssetsInDirectory(s_AssetsDirectory, false, false);
                timer.Log();
            }
            else
                HE_ENGINE_LOG_INFO("Assets directory not specified, skipping registration");
        #endif

        s_Initialized = true;

        HE_ENGINE_LOG_DEBUG("Asset manager ready");
    }

    void AssetManager::Shutdown()
    {
        s_Initialized = false;

        auto lock = std::unique_lock(s_Mutex);

        // cleanup assets
        for (auto& pair : s_Registry)
            pair.second.Asset.reset();

        // cleanup resources
        for (auto& pair : s_Resources)
            pair.second.Asset.reset();
    }

    Task AssetManager::UnloadOldAssets()
    {
        HE_PROFILE_FUNCTION();

        if (s_UnloadTask.IsValid())
            return s_UnloadTask;

        return TaskManager::Schedule([]()
        {
            auto lock = std::shared_lock(s_Mutex);

            // Check to see if assets should be unloaded
            for (auto& pair : s_UUIDs)
            {
                auto& uuidEntry = pair.second;
                auto& entry = GetRegistry(uuidEntry.IsResource)[uuidEntry.Path];

                if (entry.Persistent || !entry.Asset->IsLoaded()) continue;

                bool notRecentlyUsed = App::Get().GetFrameCount() > entry.Asset->GetLoadedFrame() + s_AssetFrameLimit;
                if (notRecentlyUsed && entry.Asset->ShouldUnload())
                {
                    UUID uuid = pair.first;
                    TaskManager::Schedule([uuid]()
                    {
                        auto lock = std::shared_lock(s_Mutex);

                        auto found = s_UUIDs.find(uuid);
                        if (found == s_UUIDs.end()) return;

                        auto& entry = GetRegistry(found->second.IsResource)[found->second.Path];

                        HE_ENGINE_LOG_TRACE(
                            "Unloading {0} @ {1}",
                            found->second.IsResource ? "resource" : "asset", 
                            entry.Asset->GetPath().Data()
                        );
                    
                        entry.Asset->Unload();
                    }, Task::Priority::Low, "Unload asset");
                }
            }

            s_UnloadTask = Task();
        }, Task::Priority::Low, "Check Asset Unloads"); 
    }

    void AssetManager::UnloadAllAssets()
    {
        auto lock = std::unique_lock(s_Mutex);

        for (auto& pair : s_Registry)
            pair.second.Asset->Unload();
    }

    void AssetManager::EnableFileWatcher()
    {
        #ifndef HE_PLATFORM_ANDROID
        if (s_FileWatcher) return;

            s_FileWatcher = CreateScope<efsw::FileWatcher>();
            s_UpdateListener = CreateScope<UpdateListener>();
        #endif
    }

    void AssetManager::DisableFileWatcher()
    {
        #ifndef HE_PLATFORM_ANDROID
            s_FileWatcher.reset();
            s_UpdateListener.reset();
        #endif
    }

    void AssetManager::WatchAssetDirectory()
    {
        #ifndef HE_PLATFORM_ANDROID
            s_WatchId = (int)s_FileWatcher->addWatch(
                s_AssetsDirectory.Data(),
                s_UpdateListener.get(),
                true
            );
        #endif
    }

    UUID AssetManager::RegisterAsset(Asset::Type type, const HString8& path, bool persistent, bool isResource)
    {
        if (path.IsEmpty())
            return 0;

        UUID oldUUID = GetAssetUUID(path, isResource);
        if (oldUUID != 0)
            return oldUUID;

        HString8 absolutePath = isResource
            ? std::filesystem::path(s_ResourceDirectory.Data()).append(path.Data()).generic_u8string()
            : GetAbsolutePath(path);

        // Only register if the path exists (resources are assumed to exist)
        if (!isResource && !FilesystemUtils::FileExistsLocalized(absolutePath.Data()))
            return 0;

        auto lock = std::unique_lock(s_Mutex);

        UUID newUUID = UUID();
        HE_ENGINE_LOG_TRACE(
            "Registering {0} {1} @ {2}",
            HE_ENUM_TO_STRING(Asset, type),
            isResource ? "resource" : "asset",
            path.Data()
        );
        GetRegistry(isResource)[path] = {
            Asset::Create(type, path, absolutePath),
            persistent,
            newUUID
        };
        s_UUIDs[newUUID] = { path, isResource, type };

        return newUUID;    
    }

    void AssetManager::UnregisterAsset(UUID uuid)
    {
        auto lock = std::unique_lock(s_Mutex);
        auto found = s_UUIDs.find(uuid);
        if (found == s_UUIDs.end()) return;
        GetRegistry(found->second.IsResource).erase(found->second.Path);
        s_UUIDs.erase(uuid);
    }

    UUID AssetManager::RegisterInMemoryAsset(Asset::Type type)
    {
        auto lock = std::unique_lock(s_Mutex);

        UUID newUUID = UUID();

        // UUID as a string should be sufficient for a unique
        // place in the registry
        HString8 idPath = std::to_string(newUUID);

        // Mark the asset's data as loaded & valid so that it never tries
        // to reload it (and fail)
        auto newAsset = Asset::Create(type, "", "");
        newAsset->m_Loaded = true;
        newAsset->m_Valid = true;

        // Register the 'loaded' asset as persistent so that it
        // never tries to unload itself 
        s_Resources[idPath] = { newAsset, true };
        s_UUIDs[newUUID] = { idPath, true, type };

        return newUUID;
    }

    void AssetManager::RegisterAssetsInDirectory(const HStringView8& directory, bool persistent, bool isResource)
    {
        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(directory.Data()))
                if (entry.is_directory())
                    RegisterAssetsInDirectory(entry.path().generic_u8string(), persistent, isResource);
                else
                {
                    std::filesystem::path path = entry.path();
                    Asset::Type type = DeduceAssetTypeFromFile(path.generic_u8string());
                    if (type != Asset::Type::None)
                    {
                        // remove the base from the start of the path so it doesn't need to be included when referencing
                        HString8 basePath = path.lexically_relative(isResource ? s_ResourceDirectory.Data() : s_AssetsDirectory.Data()).generic_u8string();
                        RegisterAsset(type, basePath, persistent, isResource);
                    }
                }
        }
        catch (std::exception e) // likely invalid path so stop searching
        { return; }
    }

    void AssetManager::RegisterAssetsFromManifest(const HStringView8& directory)
    {
        u32 fileLength;
        auto path = std::filesystem::path(directory.Data()).append(s_ManifestFile.Data());
        unsigned char* data = FilesystemUtils::ReadFile(path.generic_u8string(), fileLength);
        if (!data)
        {
            HE_LOG_ERROR("Failed to locate manifest file in {0}", path.generic_u8string());
            return;
        }

        auto j = nlohmann::json::parse(data);
        for (auto& entry : j)
            RegisterAsset(entry["type"], entry["path"], false, entry["resource"]);
    }

    // Non-resources only
    void AssetManager::RenameAsset(const HString8& oldPath, const HString8& newPath)
    {
        auto lock = std::unique_lock(s_Mutex);

        auto found = s_Registry.find(oldPath);
        if (found == s_Registry.end()) return;

        // Update UUID link
        auto& uuidEntry = s_UUIDs[found->second.Id];
        if (uuidEntry.IsResource) // Cannot update resources
            return;
        uuidEntry.Path = newPath;

        // Update asset path
        found->second.Asset->UpdatePath(newPath, GetAbsolutePath(newPath));

        // Change registry key
        auto registryNode = s_Registry.extract(oldPath);
        registryNode.key() = newPath;
        s_Registry.insert(std::move(registryNode));
    }

    void AssetManager::UpdateAssetsDirectory(const HString8& directory)
    {
        if (directory == s_AssetsDirectory)
            return;

        // (Locks internally)
        UnloadAllAssets();

        s_Mutex.lock();

        s_AssetsDirectory = directory;

        if (!s_Initialized)
        {
            s_Mutex.unlock();
            return;
        }

        // Remove all registry UUIDs
        for (auto& pair : s_Registry)
            s_UUIDs.erase(pair.second.Id);

        // Clear all the registered assets
        s_Registry.clear();

        s_Mutex.unlock();

        // Scan the new directory
        #ifdef HE_DIST
            RegisterAssetsFromManifest(directory);
        #else
            RegisterAssetsInDirectory(directory, false, false);
        #endif

        #ifndef HE_PLATFORM_ANDROID
            // Update file watcher to watch new directory
            if (s_FileWatcher)
            {
                s_FileWatcher->removeWatch(s_WatchId);
                WatchAssetDirectory();
            }
        #endif
    }

    Asset::Type AssetManager::DeduceAssetTypeFromFile(const HStringView8& path)
    {
        auto extension = std::filesystem::path(path.Data()).extension().generic_u8string();

        // Convert the extension to lowercase
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });
        
        Asset::Type type = Asset::Type::None;
        if (extension == ".png" || extension == ".jpg" || extension == ".bmp" ||
            extension == ".hdr" || extension == ".tiff" || extension == ".tif") // textures
            type = Asset::Type::Texture;
        else if (extension == ".gltf")
            type = Asset::Type::Mesh;
        else if (extension == ".vert" || extension == ".frag" || extension == ".comp" ||
                 extension == ".rgen" || extension == ".rmiss" || extension == ".rchit" ||
                 extension == ".rint" || extension == ".rahit")
            type = Asset::Type::Shader;
        else if (extension == ".hemat")
            type = Asset::Type::Material;
        else if (extension == ".hescn")
            type = Asset::Type::Scene;
        else if (extension == ".ttf" || extension == ".otf")
            type = Asset::Type::Font;
        else if (extension == ".splat")
            type = Asset::Type::Splat;

        return type;
    }

    UUID AssetManager::GetAssetUUID(const HString8& path, bool isResource)
    {
        auto lock = std::shared_lock(s_Mutex);
        auto& registry = GetRegistry(isResource);
        auto found = registry.find(path);
        if (found != registry.end())
            return found->second.Id;
        return 0;
    }

    HString8 AssetManager::GetPathFromUUID(UUID uuid)
    {
        if (!uuid) return "";
        auto lock = std::shared_lock(s_Mutex);
        auto found = s_UUIDs.find(uuid);
        if (found == s_UUIDs.end()) return "";
        return found->second.Path;
    }

    bool AssetManager::IsAssetAResource(UUID uuid)
    {
        if (!uuid) return false;
        auto lock = std::shared_lock(s_Mutex);
        auto found = s_UUIDs.find(uuid);
        if (found == s_UUIDs.end()) return "";
        return found->second.IsResource;
    }

    Asset* AssetManager::RetrieveAsset(const HString8& path, bool isResource)
    {
        if (path.IsEmpty()) return nullptr;

        auto lock = std::shared_lock(s_Mutex);

        auto& registry = GetRegistry(isResource);
        auto found = registry.find(path);
        if (found == registry.end()) return nullptr;

        return found->second.Asset.get();
    }

    Asset* AssetManager::RetrieveAsset(UUID uuid)
    {
        HE_PROFILE_FUNCTION();

        if (!uuid) return nullptr;

        auto lock = std::shared_lock(s_Mutex);

        auto found = s_UUIDs.find(uuid);
        if (found == s_UUIDs.end()) return nullptr;

        auto& entry = GetRegistry(found->second.IsResource)[found->second.Path];
        return entry.Asset.get();
    }

    nlohmann::json AssetManager::GenerateManifest()
    {
        nlohmann::json j;

        u32 size = 0;
        for (auto& entry : s_UUIDs)
        {
            auto& elem = j[size++];
            elem["path"] = entry.second.Path;
            elem["type"] = entry.second.Type;
            elem["resource"] = entry.second.IsResource;
        }

        return j;
    }
}
