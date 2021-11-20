#pragma once

#include "Heart/Core/UUID.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace Heart
{
    struct MaterialData
    {
        inline void SetBaseColor(glm::vec4 color) { BaseColor = color; }
        inline void SetTexCoordScale(glm::vec2 scale) { TexCoordTransform.x = scale.x; TexCoordTransform.y = scale.y; }
        inline void SetTexCoordOffset(glm::vec2 offset) { TexCoordTransform.z = offset.x; TexCoordTransform.w = offset.y; }
        inline void SetHasAlbedo(bool flag) { HasTextures.x = flag; }
        inline void SetHasMetallicRoughness(bool flag) { HasTextures.y = flag; }
        inline void SetHasNormal(bool flag) { HasTextures.z = flag; }
        inline void SetMetalnessFactor(float factor) { Scalars.x = factor; }
        inline void SetRoughnessFactor(float factor) { Scalars.y = factor; }

        inline glm::vec4 GetBaseColor() const { return BaseColor; }
        inline glm::vec2 GetTexCoordScale() const { return { TexCoordTransform.x, TexCoordTransform.y }; }
        inline glm::vec2 GetTexCoordOffset() const { return { TexCoordTransform.z, TexCoordTransform.w }; }
        inline bool HasAlbedo() const { return HasTextures.x; }
        inline bool HasMetallicRoughness() const { return HasTextures.y; }
        inline bool HasNormal() const { return HasTextures.z; }
        inline float GetMetalnessFactor() const { return Scalars.x; }
        inline float GetRoughnessFactor() const { return Scalars.y; }

        glm::vec4 BaseColor = { 1.f, 1.f, 1.f, 1.f };
        glm::vec4 TexCoordTransform = { 1.f, 1.f, 0.f, 0.f }; // [0-1]: scale, [2-3]: offset
        glm::vec4 HasTextures = { 0, 0, 0, 0 }; // [0]: hasAlbedo, [1]: hasMetallicRoughness, [2]: hasNormal
        glm::vec4 Scalars = { 0.f, 0.f, 0.f, 0.f }; // [0]: metalness, [1]: roughness
    };

    class Material
    {
    public:
    
        inline MaterialData& GetMaterialData() { return m_MaterialData; }
        inline UUID GetAlbedoTexture() const { return m_AlbedoTextureAsset; }
        inline UUID GetMetallicRoughnessTexture() const { return m_MetallicRoughnessTextureAsset; }
        inline UUID GetNormalTexture() const { return m_NormalTextureAsset; }
        inline bool IsTransparent() const { return m_Transparent; }

    private:
        MaterialData m_MaterialData;
        UUID m_AlbedoTextureAsset = 0;
        UUID m_MetallicRoughnessTextureAsset = 0;
        UUID m_NormalTextureAsset = 0;
        bool m_Transparent = false;

        friend class MaterialAsset;
        friend class MeshAsset;
    };
}