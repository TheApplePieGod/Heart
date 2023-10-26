#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString8.h"
#include "Heart/Task/Task.h"

namespace Heart
{
    class Asset
    {
    public:
        /*! @brief All of the supported asset types. */
        enum class Type
        {
            None = 0,
            Texture, Shader, Mesh, Material, Scene, Font
        };

        /*! @brief Debug strings for each asset type. */
        inline static const char* TypeStrings[] = {
            "None", "Texture", "Shader", "Mesh", "Material", "Scene", "Font"
        };

    public:
        /**
         * @brief Default constructor.
         * 
         * @param path The path of the asset relative to the project directory.
         * @param absolutePath The absolute filesystem path of the asset.
         */
        Asset(const HString8& path, const HString8& absolutePath);

        void Load();
        void Unload();

        /**
         * @brief Update the relative and absolute paths of this asset.
         * 
         * @note This will not reload the data.
         * 
         * @param path The path of the asset relative to the project directory.
         * @param absolutePath The absolute filesystem path of the asset.
         */
        void UpdatePath(const HString8& path, const HString8& absolutePath);

        Asset* LoadImmediate();
        Asset* EnsureValid();

        /**
         * @brief Check if the asset has been loaded.
         * 
         * This will will be true even if the load was unsuccessful, so make sure
         * to check IsValid() as well.
         */
        inline bool IsLoaded() const { return m_Loaded; }

        /*! @brief Check if the asset successfully loaded its data from disk. */
        inline bool IsValid() const { return m_Valid; }

        /*! @brief Return the last frame that this asset was loaded. */
        inline u64 GetLoadedFrame() const { return m_LoadedFrame; }

        inline const HString8& GetPath() const { return m_Path; }
        inline const HString8& GetAbsolutePath() const { return m_AbsolutePath; }
        inline const HString8& GetFilename() const { return m_Filename; }
        inline Type GetType() const { return m_Type; }

    public:
        /**
         * @brief Statically create a new asset object.
         * 
         * @param type The asset type.
         * @param path The path of the asset relative to the project directory.
         * @param absolutePath The absolute filesystem path of the asset.
         * @return A ref to a new asset object.
         */
        static Ref<Asset> Create(Type type, const HStringView8& path, const HStringView8& absolutePath);

        static void LoadMany(const std::initializer_list<Asset*>& assets);
        static void LoadMany(const HVector<Asset*>& assets);

    protected:
        virtual void LoadInternal() = 0;
        virtual void UnloadInternal() = 0;

    protected:
        HString8 m_Path;
        HString8 m_AbsolutePath;
        HString8 m_ParentPath;
        HString8 m_Filename;
        HString8 m_Extension;
        void* m_Data = nullptr;
        Type m_Type = Type::None;

        std::atomic<bool> m_Valid = false;
        std::atomic<bool> m_Loaded = false;

    private:
        void UpdateLoadStatus();

    private:
        std::mutex m_LoadLock;
        std::atomic<u64> m_LoadedFrame = 0;
        Task m_LoadingTask;

        friend class AssetManager;
    };
}
