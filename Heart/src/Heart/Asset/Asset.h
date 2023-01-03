#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString8.h"

namespace Heart
{
    /**
     * @brief The base class for all assets that flow through the engine.
     * 
     * Each asset is created with a set type, which determines how that asset loads its data.
     * An asset can be permanent, and the data contained inside the asset can have
     * its lifecycle managed outside the asset (i.e. AssetManager).
     */
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
        Asset(const HStringView8& path, const HStringView8& absolutePath);

        /*! @brief Load the asset's data. */
        virtual void Load(bool async = false) = 0;

        /*! @brief Unload the asset's data. */
        virtual void Unload() = 0;

        /*! @brief Reload the asset's data. */
        void Reload();

        /**
         * @brief Update the relative and absolute paths of this asset.
         * 
         * @note This will not reload the data.
         * 
         * @param path The path of the asset relative to the project directory.
         * @param absolutePath The absolute filesystem path of the asset.
         */
        void UpdatePath(const HStringView8& path, const HStringView8& absolutePath);

        /*! @brief Get the asset's relative path. */
        inline const HString8& GetPath() const { return m_Path; }

        /*! @brief Get the asset's absolute path. */
        inline const HString8& GetAbsolutePath() const { return m_AbsolutePath; }

        /*! @brief Get the asset's filename. */
        inline const HString8& GetFilename() const { return m_Filename; }

        /**
         * @brief Check if the asset has been loaded.
         * 
         * This will will be true even if the load was unsuccessful, so make sure
         * to check IsValid() as well.
         */
        inline bool IsLoaded() const { return m_Loaded; }

        /*! @brief Check if the asset is in the process of loading. */
        inline bool IsLoading() const { return m_Loading; }

        /*! @brief Check if the asset successfully loaded its data from disk. */
        inline bool IsValid() const { return m_Valid; }

        /*! @brief Get the asset's type. */
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

        /**
         * @brief Convert a base64 string into an array of bytes.
         * 
         * @param encoded The encoded string.
         * @return A vector containing the decoded bytes.
         */
        static HVector<unsigned char> Base64Decode(const HStringView8& encoded);

        /**
         * @brief Determine if a given character is base64.
         * 
         * @param c The character to check.
         * @return True if the character is base64, false otherwise.
         */
        inline static bool IsBase64(unsigned char c) { return (isalnum(c) || (c == '+') || (c == '/')); }

    protected:
        HString8 m_Path;
        HString8 m_AbsolutePath;
        HString8 m_ParentPath;
        HString8 m_Filename;
        HString8 m_Extension;
        void* m_Data = nullptr;
        bool m_Loaded = false;
        bool m_Loading = false;
        bool m_Valid = false;
        Type m_Type = Type::None;

    protected:
        static inline const HString8 s_Base64Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

    private:
        friend class AssetManager;
    };
}
