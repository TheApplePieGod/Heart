#pragma once

#include "Heart/Core/UUID.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace Flourish
{
    class ResourceSet;
    class ResourceSetAllocator;
}

namespace Heart
{
    enum class TransparencyMode
    {
        Opaque = 0,
        AlphaClip,
        AlphaBlend
    };

    struct MaterialData
    {
        inline void SetBaseColor(glm::vec4 color) { BaseColor = color; }
        inline void SetEmissiveFactor(glm::vec4 factor) { EmissiveFactor = factor; }
        inline void SetTexCoordScale(glm::vec2 scale) { TexCoordTransform.x = scale.x; TexCoordTransform.y = scale.y; }
        inline void SetTexCoordOffset(glm::vec2 offset) { TexCoordTransform.z = offset.x; TexCoordTransform.w = offset.y; }
        inline void SetHasAlbedo(bool flag) { HasPBRTextures.x = flag; }
        inline void SetHasMetallicRoughness(bool flag) { HasPBRTextures.y = flag; }
        inline void SetHasNormal(bool flag) { HasTextures.x = flag; }
        inline void SetHasEmissive(bool flag) { HasTextures.y = flag; }
        inline void SetHasOcclusion(bool flag) { HasTextures.z = flag; }
        inline void SetMetalnessFactor(float factor) { Scalars.x = factor; }
        inline void SetRoughnessFactor(float factor) { Scalars.y = factor; }
        inline void SetAlphaClipThreshold(float threshold) { Scalars.z = threshold; }

        inline glm::vec4 GetBaseColor() const { return BaseColor; }
        inline glm::vec4 GetEmissiveFactor() const { return EmissiveFactor; }
        inline glm::vec2 GetTexCoordScale() const { return { TexCoordTransform.x, TexCoordTransform.y }; }
        inline glm::vec2 GetTexCoordOffset() const { return { TexCoordTransform.z, TexCoordTransform.w }; }
        inline bool HasAlbedo() const { return HasPBRTextures.x; }
        inline bool HasMetallicRoughness() const { return HasPBRTextures.y; }
        inline bool HasNormal() const { return HasTextures.x; }
        inline bool HasEmissive() const { return HasTextures.y; }
        inline bool HasOcclusion() const { return HasTextures.z; }
        inline float GetMetalnessFactor() const { return Scalars.x; }
        inline float GetRoughnessFactor() const { return Scalars.y; }
        inline float GetAlphaClipThreshold() const { return Scalars.z; }

        glm::vec4 BaseColor = { 1.f, 1.f, 1.f, 1.f };
        glm::vec4 EmissiveFactor = { 0.f, 0.f, 0.f, 0.f };
        glm::vec4 TexCoordTransform = { 1.f, 1.f, 0.f, 0.f }; // [0-1]: scale, [2-3]: offset
        glm::vec4 HasPBRTextures = { 0, 0, 0, 0 }; // [0]: hasAlbedo, [1]: hasMetallicRoughness
        glm::vec4 HasTextures = { 0, 0, 0, 0 }; // [0]: hasNormal, [1]: hasEmissive, [2]: hasOcclusion
        glm::vec4 Scalars = { 0.f, 1.f, 0.f, 0.f }; // [0]: metalness, [1]: roughness, [2]: alphaClipThreshold
    };

    class Material
    {
    public:
        void RecomputeResourceSet();

        inline const Flourish::ResourceSet* GetResourceSet() const { return m_ResourceSet.get(); }
        inline MaterialData& GetMaterialData() { return m_MaterialData; }
        inline UUID GetAlbedoTexture() const { return m_AlbedoTextureAsset; }
        inline UUID GetMetallicRoughnessTexture() const { return m_MetallicRoughnessTextureAsset; }
        inline UUID GetNormalTexture() const { return m_NormalTextureAsset; }
        inline UUID GetEmissiveTexture() const { return m_EmissiveTextureAsset; }
        inline UUID GetOcclusionTexture() const { return m_OcclusionTextureAsset; }
        inline TransparencyMode GetTransparencyMode() const { return m_TransparencyMode; }

        inline void SetAlbedoTexture(UUID texture) { m_AlbedoTextureAsset = texture; }
        inline void SetMetallicRoughnessTexture(UUID texture) { m_MetallicRoughnessTextureAsset = texture; }
        inline void SetNormalTexture(UUID texture) { m_NormalTextureAsset = texture; }
        inline void SetEmissiveTexture(UUID texture) { m_EmissiveTextureAsset = texture; }
        inline void SetOcclusionTexture(UUID texture) { m_OcclusionTextureAsset = texture; }
        inline void SetTransparencyMode(TransparencyMode mode) { m_TransparencyMode = mode; }

    public:
        static void Initialize();
        static void Shutdown();
    
    private:
        inline static Ref<Flourish::ResourceSetAllocator> s_ResourceSetAllocator = nullptr;

    private:
        MaterialData m_MaterialData;
        UUID m_AlbedoTextureAsset = 0;
        UUID m_MetallicRoughnessTextureAsset = 0;
        UUID m_NormalTextureAsset = 0;
        UUID m_EmissiveTextureAsset = 0;
        UUID m_OcclusionTextureAsset = 0;
        TransparencyMode m_TransparencyMode = TransparencyMode::Opaque;

        Ref<Flourish::ResourceSet> m_ResourceSet;

        friend class MaterialAsset;
        friend class MeshAsset;
    };
}
