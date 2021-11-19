#pragma once

#include "Heart/Core/UUID.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

namespace Heart
{
    struct MaterialData
    {
        glm::vec4 BaseColor = { 1.f, 1.f, 1.f, 1.f };
        float Roughness = 0.5f;
        float Metalness = 0.5f;
        glm::vec2 TexCoordScale = { 1.f, 1.f };
        glm::vec2 TexCoordOffset = { 0.f, 0.f };
        bool HasAlbedo = false;
        bool HasRoughness = false;
        bool HasMetalness = false;
        bool HasNormal = false;
        glm::vec2 padding = { 0.f, 0.f };
    };

    class Material
    {
    public:
    
        inline MaterialData& GetMaterialData() { return m_MaterialData; }
        inline UUID GetAlbedoTexture() const { return m_AlbedoTextureAsset; }
        inline UUID GetRoughnessTexture() const { return m_RoughnessTextureAsset; }
        inline UUID GetMetalnessTexture() const { return m_MetalnessTextureAsset; }
        inline UUID GetNormalTexture() const { return m_NormalTextureAsset; }
        inline bool IsTransparent() const { return m_Transparent; }

    private:
        MaterialData m_MaterialData;
        UUID m_AlbedoTextureAsset = 0;
        UUID m_RoughnessTextureAsset = 0;
        UUID m_MetalnessTextureAsset = 0;
        UUID m_NormalTextureAsset = 0;
        bool m_Transparent = false;

        friend class MaterialAsset;
        friend class MeshAsset;
    };
}