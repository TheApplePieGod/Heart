#include "htpch.h"
#include "MaterialAsset.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Asset/AssetManager.h"
#include "nlohmann/json.hpp"

namespace Heart
{
    void MaterialAsset::Load()
    {
        if (m_Loaded) return;

        try
        {
            m_Material = DeserializeMaterial(m_AbsolutePath);
        }
        catch (std::exception e)
        {
            HE_ENGINE_LOG_ERROR("Failed to load material at path {0}", m_AbsolutePath);
            m_Loaded = true;
            return;
        }

        m_Data = nullptr;
        m_Loaded = true;
        m_Valid = true;
    }

    void MaterialAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Material = Material();

        m_Data = nullptr;
        m_Loaded = false;
        m_Valid = false;
    }

    Material MaterialAsset::DeserializeMaterial(const std::string& path)
    {
        Material material;

        u32 fileLength;
        unsigned char* data = FilesystemUtils::ReadFile(path, fileLength);
        auto j = nlohmann::json::parse(data);

        // parse material data
        {
            auto& field = j["data"];
            material.m_MaterialData.BaseColor = { field["baseColor"][0], field["baseColor"][1], field["baseColor"][2], field["baseColor"][3] };
            material.m_MaterialData.Roughness = field["roughness"];
            material.m_MaterialData.Metalness = field["metalness"];
            material.m_MaterialData.TexCoordScale = { field["texCoordScale"][0], field["texCoordScale"][1] };
            material.m_MaterialData.TexCoordOffset = { field["texCoordOffset"][0], field["texCoordOffset"][1] };
        }

        // parse metadata
        {
            auto& field = j["metadata"];
            material.m_Transparent = field["transparent"];
        }

        // parse texture data
        {
            auto& field = j["textures"];
            material.m_AlbedoTextureAsset = AssetManager::RegisterAsset(Asset::Type::Texture, field["albedo"]);
            material.m_RoughnessTextureAsset = AssetManager::RegisterAsset(Asset::Type::Texture, field["roughness"]);
            material.m_MetalnessTextureAsset = AssetManager::RegisterAsset(Asset::Type::Texture, field["metalness"]);
            material.m_NormalTextureAsset = AssetManager::RegisterAsset(Asset::Type::Texture, field["normal"]);
        }

        delete[] data;
        return material;
    }

    void MaterialAsset::SerializeMaterial(const std::string& path, const Material& material)
    {
        nlohmann::json j;

        // material data
        {
            auto& field = j["data"];
            field["baseColor"] = nlohmann::json::array({ material.m_MaterialData.BaseColor.r, material.m_MaterialData.BaseColor.g, material.m_MaterialData.BaseColor.b, material.m_MaterialData.BaseColor.a });
            field["roughness"] = material.m_MaterialData.Roughness;
            field["metalness"] = material.m_MaterialData.Metalness;
            field["texCoordScale"] = nlohmann::json::array({ material.m_MaterialData.TexCoordScale.x, material.m_MaterialData.TexCoordScale.y });
            field["texCoordOffset"] = nlohmann::json::array({ material.m_MaterialData.TexCoordOffset.x, material.m_MaterialData.TexCoordOffset.y });
        }

        // metadata
        {
            auto& field = j["metadata"];
            field["transparent"] = material.m_Transparent;
        }

        // texture data
        {
            auto& field = j["textures"];
            field["albedo"] = AssetManager::GetPathFromUUID(material.m_AlbedoTextureAsset);
            field["roughness"] = AssetManager::GetPathFromUUID(material.m_RoughnessTextureAsset);
            field["metalness"] = AssetManager::GetPathFromUUID(material.m_MetalnessTextureAsset);
            field["normal"] = AssetManager::GetPathFromUUID(material.m_NormalTextureAsset);
        }

        std::ofstream file(path);
        file << j;
    }
}