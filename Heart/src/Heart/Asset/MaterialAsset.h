#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Renderer/Material.h"

namespace Heart
{
    class MaterialAsset : public Asset
    {
    public:
        MaterialAsset(const std::string& path, const std::string& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Material; }

        void Load() override;
        void Unload() override;

        Material& GetMaterial() { return m_Material; }

    public:
        static Material DeserializeMaterial(const std::string& path);
        static void SerializeMaterial(const std::string& path, const Material& material);

    private:
        Material m_Material;
    };
}