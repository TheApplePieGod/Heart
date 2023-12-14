#include "hepch.h"
#include "Asset.h"

#include "Heart/Core/App.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Asset/FontAsset.h"
#include "Heart/Asset/SplatAsset.h"

namespace Heart
{
    Asset::Asset(const HString8& path, const HString8& absolutePath)
        : m_Path(path), m_AbsolutePath(absolutePath)
    {
        UpdatePath(path, absolutePath);
    }

    // TODO: assets reloading
    Asset* Asset::Load(bool wait)
    {
        if (wait)
        {
            std::lock_guard lock(m_LoadLock);

            if (m_Loaded)
            {
                UpdateLoadedFrame();
                return this;
            }
            
            // We don't create a task here because when we want to ensure the
            // loaded state it is always better to just load immediately rather
            // than waiting for a task
            LoadInternal();
            UpdateLoadStatus();

            return this;
        }

        // If the lock is locked, someone else is handling load so we
        // don't have to do anything
        if (!m_LoadLock.try_lock())
            return this;

        if (m_Loaded || m_LoadingTask.IsValid())
        {
            UpdateLoadedFrame();
            m_LoadLock.unlock();
            return this;
        }

        // TODO: safety for when asset object goes out of scope

        // Only one load task should ever get created at a time
        m_LoadingTask = TaskManager::Schedule([this]()
        {
            std::lock_guard lock(m_LoadLock);

            if (m_Loaded) return;

            LoadInternal();
            UpdateLoadStatus();
        }, Task::Priority::Medium, "Asset Load");

        m_LoadLock.unlock();

        return this;
    }

    // TODO: revisit this
    void Asset::Unload()
    {
        std::lock_guard lock(m_LoadLock);

        if (!m_Loaded) return;

        UnloadInternal();
        m_Loaded = false;
        m_Valid = false;
    }

    Asset* Asset::EnsureValid()
    {
        std::lock_guard lock(m_LoadLock);

        if (!m_Loaded)
        {
            LoadInternal();
            UpdateLoadStatus();
        }

        if (!m_Valid)
        {
            HE_ENGINE_LOG_CRITICAL(
                "Asset '{0}' expected to be valid and is not",
                m_Path.Data()
            );
            HE_ENGINE_ASSERT(false);

            return nullptr;
        }

        return this;
    }

    void Asset::UpdateLoadedFrame()
    {
        m_LoadedFrame = Heart::App::Get().GetFrameCount();
    }

    void Asset::UpdateLoadStatus()
    {
        UpdateLoadedFrame();
        m_Loaded = true;
        m_LoadingTask = Task();
    }

    void Asset::UpdatePath(const HString8& path, const HString8& absolutePath)
    {
        auto entry = std::filesystem::path(path.Data());
        m_Filename = entry.filename().generic_u8string();
        m_Extension = entry.extension().generic_u8string();
        m_ParentPath = entry.parent_path().generic_u8string();

        // convert the extension to lowercase
        std::transform((char*)m_Extension.Begin(), (char*)m_Extension.End(), (char*)m_Extension.Begin(), [](unsigned char c) { return std::tolower(c); });

        m_Path = path;
        m_AbsolutePath = absolutePath;
    }

    Ref<Asset> Asset::Create(Type type, const HString8& path, const HString8& absolutePath)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create asset: selected type is not supported"); return nullptr; }
            case Asset::Type::Texture:
            { return CreateRef<TextureAsset>(path, absolutePath); }
            case Asset::Type::Shader:
            { return CreateRef<ShaderAsset>(path, absolutePath); }
            case Asset::Type::Mesh:
            { return CreateRef<MeshAsset>(path, absolutePath); }
            case Asset::Type::Material:
            { return CreateRef<MaterialAsset>(path, absolutePath); }
            case Asset::Type::Scene:
            { return CreateRef<SceneAsset>(path, absolutePath); }
            case Asset::Type::Font:
            { return CreateRef<FontAsset>(path, absolutePath); }
            case Asset::Type::Splat:
            { return CreateRef<SplatAsset>(path, absolutePath); }
        }
    }

    void Asset::LoadMany(const std::initializer_list<Asset*>& assets, bool wait)
    {
        for (auto& asset : assets)
            asset->Load(wait);
    }

    void Asset::LoadMany(const HVector<Asset*>& assets, bool wait)
    {
        for (auto& asset : assets)
            asset->Load(wait);
    }
}
