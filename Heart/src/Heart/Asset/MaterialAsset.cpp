#include "hepch.h"
#include "MaterialAsset.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Asset/AssetManager.h"
#include "nlohmann/json.hpp"

namespace Heart
{
    void MaterialAsset::Load(bool async)
    {
        if (m_Loaded || m_Loading) return;
        m_Loading = true;

        try
        {
            m_Material = DeserializeMaterial(m_AbsolutePath);
        }
        catch (std::exception e)
        {
            HE_ENGINE_LOG_ERROR("Failed to load material at path {0}", m_AbsolutePath.Data());
            m_Loaded = true;
            m_Loading = false;
            return;
        }

        m_Data = nullptr;
        m_Loaded = true;
        m_Loading = false;
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

    void MaterialAsset::Save(const Material& material)
    {
        SerializeMaterial(m_AbsolutePath, material);
        if (m_Loaded)
            m_Material = material;
    }

    Material MaterialAsset::DeserializeMaterial(const HStringView8& path)
    {
        Material material;

        u32 fileLength;
        unsigned char* data = FilesystemUtils::ReadFile(path, fileLength);
        if (!data)
            throw std::exception();

        auto j = nlohmann::json::parse(data);

        // parse material data
        {
            auto& field = j["data"];
            if (field.contains("baseColor"))
                material.m_MaterialData.SetBaseColor({ field["baseColor"][0], field["baseColor"][1], field["baseColor"][2], field["baseColor"][3] });
            if (field.contains("emissiveFactor"))
                material.m_MaterialData.SetEmissiveFactor({ field["emissiveFactor"][0], field["emissiveFactor"][1], field["emissiveFactor"][2], 0.f });
            if (field.contains("roughness"))
                material.m_MaterialData.SetRoughnessFactor(field["roughness"]);
            if (field.contains("metalness"))
                material.m_MaterialData.SetMetalnessFactor(field["metalness"]);
            if (field.contains("alphaClipThreshold"))
                material.m_MaterialData.SetAlphaClipThreshold(field["alphaClipThreshold"]);
            if (field.contains("texCoordScale"))
                material.m_MaterialData.SetTexCoordScale({ field["texCoordScale"][0], field["texCoordScale"][1] });
            if (field.contains("texCoordOffset"))
                material.m_MaterialData.SetTexCoordOffset({ field["texCoordOffset"][0], field["texCoordOffset"][1] });
        }

        // parse metadata
        {
            auto& field = j["metadata"];
            if (field.contains("translucent"))
                material.m_Translucent = field["translucent"];
        }

        // parse texture data
        {
            auto& field = j["textures"];
            if (field.contains("albedo"))
                material.m_AlbedoTextureAsset = AssetManager::RegisterAsset(Asset::Type::Texture, field["albedo"]["path"], false, field["albedo"]["engineResource"]);
            if (field.contains("metallicRoughness"))
                material.m_MetallicRoughnessTextureAsset = AssetManager::RegisterAsset(Asset::Type::Texture, field["metallicRoughness"]["path"], false, field["metallicRoughness"]["engineResource"]);
            if (field.contains("normal"))
                material.m_NormalTextureAsset = AssetManager::RegisterAsset(Asset::Type::Texture, field["normal"]["path"], false, field["normal"]["engineResource"]);
            if (field.contains("emissive"))
                material.m_EmissiveTextureAsset = AssetManager::RegisterAsset(Asset::Type::Texture, field["emissive"]["path"], false, field["emissive"]["engineResource"]);
            if (field.contains("occlusion"))
                material.m_OcclusionTextureAsset = AssetManager::RegisterAsset(Asset::Type::Texture, field["occlusion"]["path"], false, field["occlusion"]["engineResource"]);
        }

        delete[] data;
        return material;
    }

    void MaterialAsset::SerializeMaterial(const HStringView8& path, const Material& material)
    {
        nlohmann::json j;

        // material data
        {
            auto& field = j["data"];
            field["baseColor"] = nlohmann::json::array({ material.m_MaterialData.GetBaseColor().r, material.m_MaterialData.GetBaseColor().g, material.m_MaterialData.GetBaseColor().b, material.m_MaterialData.GetBaseColor().a });
            field["emissiveFactor"] = nlohmann::json::array({ material.m_MaterialData.GetEmissiveFactor().r, material.m_MaterialData.GetEmissiveFactor().g, material.m_MaterialData.GetEmissiveFactor().b });
            field["roughness"] = material.m_MaterialData.GetRoughnessFactor();
            field["metalness"] = material.m_MaterialData.GetMetalnessFactor();
            field["alphaClipThreshold"] = material.m_MaterialData.GetAlphaClipThreshold();
            field["texCoordScale"] = nlohmann::json::array({ material.m_MaterialData.GetTexCoordScale().x, material.m_MaterialData.GetTexCoordScale().y });
            field["texCoordOffset"] = nlohmann::json::array({ material.m_MaterialData.GetTexCoordOffset().x, material.m_MaterialData.GetTexCoordOffset().y });
        }

        // metadata
        {
            auto& field = j["metadata"];
            field["translucent"] = material.m_Translucent;
        }

        // texture data
        {
            auto& field = j["textures"];
            field["albedo"]["path"] = AssetManager::GetPathFromUUID(material.m_AlbedoTextureAsset);
            field["albedo"]["engineResource"] = AssetManager::IsAssetAResource(material.m_AlbedoTextureAsset);

            field["metallicRoughness"]["path"] = AssetManager::GetPathFromUUID(material.m_MetallicRoughnessTextureAsset);
            field["metallicRoughness"]["engineResource"] = AssetManager::IsAssetAResource(material.m_MetallicRoughnessTextureAsset);

            field["normal"]["path"] = AssetManager::GetPathFromUUID(material.m_NormalTextureAsset);
            field["normal"]["engineResource"] = AssetManager::IsAssetAResource(material.m_NormalTextureAsset);

            field["emissive"]["path"] = AssetManager::GetPathFromUUID(material.m_EmissiveTextureAsset);
            field["emissive"]["engineResource"] = AssetManager::IsAssetAResource(material.m_EmissiveTextureAsset);

            field["occlusion"]["path"] = AssetManager::GetPathFromUUID(material.m_OcclusionTextureAsset);
            field["occlusion"]["engineResource"] = AssetManager::IsAssetAResource(material.m_OcclusionTextureAsset);
        }

        std::ofstream file(path.Data());
        file << j;
    }
}