#include "hepch.h"
#include "AssetManager.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Timing.h"
#include "Heart/Task/TaskManager.h"

namespace Heart
{
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

        // Wait for tasks to finish
        while (s_AsyncLoadsInProgress)
        { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

        // cleanup assets
        for (auto& pair : s_Registry)
            pair.second.Asset.reset();

        // cleanup resources
        for (auto& pair : s_Resources)
            pair.second.Asset.reset();
    }

    // TODO: this could probably be a task
    void AssetManager::OnUpdate()
    {
        HE_PROFILE_FUNCTION();

        // Check to see if assets should be unloaded
        for (auto& pair : s_UUIDs)
        {
            auto& uuidEntry = pair.second;
            auto& entry = uuidEntry.IsResource ? s_Resources[uuidEntry.Path] : s_Registry[uuidEntry.Path];

            if (entry.Persistent) continue;

            if (App::Get().GetFrameCount() > entry.LoadedFrame + s_AssetFrameLimit)
            {
                entry.LoadedFrame = std::numeric_limits<u64>::max() - s_AssetFrameLimit; // prevent extraneous unloading

                UUID uuid = pair.first;
                s_AsyncLoadsInProgress++;
                TaskManager::Schedule([uuid]()
                {
                    auto& uuidEntry = s_UUIDs[uuid];
                    auto& entry = uuidEntry.IsResource ? s_Resources[uuidEntry.Path] : s_Registry[uuidEntry.Path];

                    HE_ENGINE_LOG_TRACE(
                        "Unloading {0} @ {1}",
                        uuidEntry.IsResource ? "resource" : "asset", 
                        entry.Asset->GetPath().Data()
                    );
                
                    UnloadAsset(entry, true);
                    s_AsyncLoadsInProgress--;
                }, Task::Priority::Low, "Unload asset");
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

    void AssetManager::LoadAsset(AssetEntry& entry, bool async)
    {
        entry.LoadedFrame = App::Get().GetFrameCount();
        entry.Asset->Load(async);
    }
    
    void AssetManager::UnloadAsset(AssetEntry& entry, bool async)
    {
        entry.LoadedFrame = std::numeric_limits<u64>::max() - s_AssetFrameLimit; // prevent extraneous unloading
        entry.Asset->Unload();
    }

    UUID AssetManager::RegisterAsset(Asset::Type type, const HStringView8& path, bool persistent, bool isResource)
    {
        if (path.IsEmpty())
            return 0;

        UUID oldUUID = GetAssetUUID(path, isResource);
        if (oldUUID != 0)
            return oldUUID;

        // Only register if the path exists (resources are assumed to exist)
        if (!isResource && !std::filesystem::exists(GetAbsolutePath(path).Data()))
            return 0;

        UUID newUUID = UUID();
        if (isResource)
        {
            if (s_Resources.find(path) == s_Resources.end())
            {  
                HE_ENGINE_LOG_TRACE("Registering {0} resource @ {1}", HE_ENUM_TO_STRING(Asset, type), path.Data());
                HString8 absolutePath = std::filesystem::path(s_ResourceDirectory.Data()).append(path.Data()).generic_u8string();
                s_Resources[path] = { Asset::Create(type, path, absolutePath), std::numeric_limits<u64>::max() - s_AssetFrameLimit, persistent };
            }
            s_UUIDs[newUUID] = { path, isResource, type };
        }
        else
        {
            if (s_Registry.find(path) == s_Registry.end())
            {
                HE_ENGINE_LOG_TRACE("Registering {0} asset @ {1}", HE_ENUM_TO_STRING(Asset, type), path.Data());
                HString8 absolutePath = std::filesystem::path(s_AssetsDirectory.Data()).append(path.Data()).generic_u8string();
                s_Registry[path] = { Asset::Create(type, path, absolutePath), std::numeric_limits<u64>::max() - s_AssetFrameLimit, persistent };
            }
            s_UUIDs[newUUID] = { path, isResource, type };
        }

        return newUUID;    
    }

    void AssetManager::UnregisterAsset(UUID uuid)
    {
        if (s_UUIDs.find(uuid) == s_UUIDs.end()) return;
        auto entry = s_UUIDs[uuid];
        s_UUIDs.erase(uuid);
        if (entry.IsResource)
            s_Resources.erase(entry.Path);
        else
            s_Registry.erase(entry.Path);
    }

    UUID AssetManager::RegisterInMemoryAsset(Asset::Type type)
    {
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
        s_Registry[idPath] = { newAsset, std::numeric_limits<u64>::max() - s_AssetFrameLimit, true };
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

    void AssetManager::RenameAsset(const HStringView8& oldPath, const HStringView8& newPath)
    {
        bool entryFound = false;
        for (auto& entry : s_UUIDs)
        {
            if (entry.second.Path == oldPath)
            {
                entry.second.Path = newPath;
                entryFound = true;
                break;
            }
        }

        if (!entryFound) return;

        // don't even bother to check for resources because those shouldn't ever be renamed

        // change registry key
        auto registryNode = s_Registry.extract(oldPath);
        registryNode.key() = newPath;
        s_Registry.insert(std::move(registryNode));

        // change asset paths
        s_Registry[newPath].Asset->UpdatePath(newPath, GetAbsolutePath(newPath));
    }

    void AssetManager::UpdateAssetsDirectory(const HStringView8& directory)
    {
        // Wait for tasks to finish
        while (s_AsyncLoadsInProgress)
        { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

        UnloadAllAssets();
        s_AssetsDirectory = directory;

        // clear all the registered assets
        s_Registry.clear();

        // remove all UUID entries that aren't resources
        for (auto it = std::begin(s_UUIDs); it != std::end(s_UUIDs);)
        {
            if (!it->second.IsResource)
                it = s_UUIDs.erase(it);
            else
                it++;
        }

        // scan the new directory
        RegisterAssetsInDirectory(directory, false, false);
    }

    Asset::Type AssetManager::DeduceAssetTypeFromFile(const HStringView8& path)
    {
        auto extension = std::filesystem::path(path.Data()).extension().generic_u8string();

        // convert the extension to lowercase
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

    UUID AssetManager::GetAssetUUID(const HStringView8& path, bool isResource)
    {
        for (auto& entry : s_UUIDs)
            if (isResource == entry.second.IsResource && entry.second.Path == path) return entry.first;
        return 0;
    }

    HString8 AssetManager::GetPathFromUUID(UUID uuid)
    {
        if (!uuid) return "";
        if (s_UUIDs.find(uuid) == s_UUIDs.end()) return "";
        return s_UUIDs[uuid].Path;
    }

    bool AssetManager::IsAssetAResource(UUID uuid)
    {
        if (!uuid) return false;
        if (s_UUIDs.find(uuid) == s_UUIDs.end()) return false;
        return s_UUIDs[uuid].IsResource;
    }

    void AssetManager::QueueLoad(AssetEntry& entry, bool async)
    {
        // Ensure that we only load if the asset is unloaded and a task hasn't been started (i.e. when the loaded frame is this value)
        if (entry.LoadedFrame == std::numeric_limits<u64>::max() - s_AssetFrameLimit)
        {
            if (async)
            {
                s_AsyncLoadsInProgress++;
                TaskManager::Schedule([&entry]()
                {
                    LoadAsset(entry, true);
                    s_AsyncLoadsInProgress--;
                }, Task::Priority::Medium, "Load asset");
            }
            else
                LoadAsset(entry);
        }
        else if (!async)
        {
            // Until we do the refactor, we need to wait until the asset is fully loaded
            while (!entry.Asset->IsLoaded()) {}
        }

        entry.LoadedFrame = App::Get().GetFrameCount();
    }

    Asset* AssetManager::RetrieveAsset(const HStringView8& path, bool isResource, bool load, bool async)
    {
        if (path.IsEmpty()) return nullptr;
        if (isResource)
        { if (s_Resources.find(path) == s_Resources.end()) return nullptr; }
        else
        { if (s_Registry.find(path) == s_Registry.end()) return nullptr; }

        auto& entry = isResource ? s_Resources[path] : s_Registry[path];
        if (load)
            QueueLoad(entry, async);

        return entry.Asset.get();
    }

    Asset* AssetManager::RetrieveAsset(UUID uuid, bool load, bool async)
    {
        HE_PROFILE_FUNCTION();

        if (!uuid) return nullptr;
        if (s_UUIDs.find(uuid) == s_UUIDs.end()) return nullptr;

        auto& uuidEntry = s_UUIDs[uuid];
        auto& entry = uuidEntry.IsResource ? s_Resources[uuidEntry.Path] : s_Registry[uuidEntry.Path];

        if (load)
            QueueLoad(entry, async);
            
        return entry.Asset.get();
    }
}
