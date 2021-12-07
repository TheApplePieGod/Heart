#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Renderer/Material.h"

namespace Heart
{
    class MaterialAsset : public Asset
    {
    public:
        /**
         * @brief Default constructor.
         * 
         * @param path The path of the asset relative to the project directory.
         * @param absolutePath The absolute filesystem path of the asset.
         */
        MaterialAsset(const std::string& path, const std::string& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Material; }

        void Load() override;
        void Unload() override;

        /*! @brief Take the current loaded material and serialize it to the underlying asset file. */
        void SaveChanges() { SerializeMaterial(m_AbsolutePath, m_Material); }

        /*! @brief Get a reference to the loaded material stored in this asset. */
        inline Material& GetMaterial() { return m_Material; }

    public:
        /**
         * @brief Load a material from disk.
         * 
         * @param path The absolute path of the material file. 
         * @return A material containing the loaded data.
         */
        static Material DeserializeMaterial(const std::string& path);

        /**
         * @brief Save a material to disk.
         * 
         * @param path The absolute path of the output file.
         * @param material The material to serialize.
         */
        static void SerializeMaterial(const std::string& path, const Material& material);

    private:
        Material m_Material;
    };
}