#pragma once

#include "Heart/Core/UUID.h"
#include "Heart/Renderer/Texture.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Renderer/Buffer.h"

#include "glm/mat4x4.hpp"
#include "glm/vec4.hpp"
#include "glm/vec3.hpp"

namespace Heart
{
    class EnvironmentMap
    {
    public:
        EnvironmentMap::EnvironmentMap(UUID mapAsset)
        : m_MapAsset(mapAsset)
        {}

        void Initialize();
        void Shutdown();

        void Recalculate();
        inline void UpdateMapAsset(UUID asset) { m_MapAsset = asset; }
        inline Texture* GetEnvironmentCubemap() { return m_EnvironmentMap.get(); }
        inline Texture* GetIrradianceCubemap() { return m_IrradianceMap.get(); }
        inline Texture* GetPrefilterCubemap() { return m_PrefilterMap.get(); }
        inline Texture* GetBRDFTexture() { return m_BRDFTexture.get(); }

    private:
        struct CubemapData
        {
            glm::mat4 Proj;
            glm::mat4 View;
            glm::vec4 Params; // [0]: roughness
        };

    private:
        UUID m_MapAsset;
    
        Ref<Texture> m_EnvironmentMap;
        Ref<Texture> m_IrradianceMap;
        Ref<Texture> m_PrefilterMap;
        Ref<Texture> m_BRDFTexture;

        std::vector<Ref<Framebuffer>> m_PrefilterFramebuffers; // one for each mip level
        Ref<Framebuffer> m_BRDFFramebuffer;
        Ref<Framebuffer> m_CubemapFramebuffer;
        Ref<Framebuffer> m_IrradianceMapFramebuffer;

        Ref<Buffer> m_CubemapDataBuffer;
        Ref<Buffer> m_FrameDataBuffer;
    };
}