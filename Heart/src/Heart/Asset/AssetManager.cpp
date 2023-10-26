#include "hepch.h"
#include "AssetManager.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Timing.h"
#include "Heart/Task/TaskManager.h"
#include "efsw/efsw.hpp"

namespace Heart
{
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

    void AssetManager::Initialize()
    {
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

                if (App::Get().GetFrameCount() > entry.Asset->GetLoadedFrame() + s_AssetFrameLimit)
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
        if (s_FileWatcher) return;

        s_FileWatcher = CreateScope<efsw::FileWatcher>();
        s_UpdateListener = CreateScope<UpdateListener>();
    }

    void AssetManager::DisableFileWatcher()
    {
        s_FileWatcher.reset();
        s_UpdateListener.reset();
    }

    void AssetManager::WatchAssetDirectory()
    {
        s_WatchId = (int)s_FileWatcher->addWatch(
            s_AssetsDirectory.Data(),
            s_UpdateListener.get(),
            true
        );
    }

    UUID AssetManager::RegisterAsset(Asset::Type type, const HString8& path, bool persistent, bool isResource)
    {
        if (path.IsEmpty())
            return 0;

        UUID oldUUID = GetAssetUUID(path, isResource);
        if (oldUUID != 0)
            return oldUUID;

        // Only register if the path exists (resources are assumed to exist)
        if (!isResource && !std::filesystem::exists(GetAbsolutePath(path).Data()))
            return 0;

        auto lock = std::unique_lock(s_Mutex);

        UUID newUUID = UUID();
        HE_ENGINE_LOG_TRACE(
            "Registering {0} {1} @ {2}",
            HE_ENUM_TO_STRING(Asset, type),
            isResource ? "resource" : "asset",
            path.Data()
        );
        HString8 absolutePath = std::filesystem::path(s_ResourceDirectory.Data()).append(path.Data()).generic_u8string();
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
        s_Registry[idPath] = { newAsset, true };
        s_UUIDs[newUUID] = { idPath, false, type };

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

    // Non-resources only
    void AssetManager::RenameAsset(const HString8& oldPath, const HString8& newPath)
    {
        auto lock = std::unique_lock(s_Mutex);

        auto found = s_Registry.find(oldPath);
        if (found == s_Registry.end()) return;

        // Update UUID link
        s_UUIDs[found->second.Id].Path = newPath;

        // Update asset path
        found->second.Asset->UpdatePath(newPath, GetAbsolutePath(newPath));

        // Change registry key
        auto registryNode = s_Registry.extract(oldPath);
        registryNode.key() = newPath;
        s_Registry.insert(std::move(registryNode));
    }

    void AssetManager::UpdateAssetsDirectory(const HString8& directory)
    {
        // (Locks internally)
        UnloadAllAssets();

        auto lock = std::unique_lock(s_Mutex);

        s_AssetsDirectory = directory;

        // Remove all registry UUIDs
        for (auto& pair : s_Registry)
            s_UUIDs.erase(pair.second.Id);

        // Clear all the registered assets
        s_Registry.clear();

        // Scan the new directory
        RegisterAssetsInDirectory(directory, false, false);

        // Update file watcher to watch new directory
        if (s_FileWatcher)
        {
            s_FileWatcher->removeWatch(s_WatchId);
            WatchAssetDirectory();
        }
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

    /*
     * GetAsset(path, isResource)
     * GetAsset(uuid, isResource)
     * LoadAssets(uuid[]);
     * LoadAssets(string[]);
     */

    // TODO: async should return a task
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
}
