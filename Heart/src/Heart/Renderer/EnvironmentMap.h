#pragma once

#include "Heart/Core/UUID.h"
#include "Heart/Container/HVector.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec4.hpp"
#include "glm/vec3.hpp"

namespace Flourish
{
    class Texture;
    class Buffer;
    class Framebuffer;
    class RenderPass;
    class CommandBuffer;
    class ResourceSet;
    class RenderGraph;
}

namespace Heart
{
    class Task;
    class EnvironmentMap
    {
    public:
        EnvironmentMap(UUID mapAsset);
        ~EnvironmentMap();

        Task Recalculate();
        inline void UpdateMapAsset(UUID asset) { m_MapAsset = asset; }
        inline UUID GetMapAsset() const { return m_MapAsset; }
        inline const Flourish::Texture* GetEnvironmentCubemap() const { return m_EnvironmentMap.Texture.get(); }
        inline const Flourish::Texture* GetIrradianceCubemap() const { return m_IrradianceMap.Texture.get(); }
        inline const Flourish::Texture* GetPrefilterCubemap() const { return m_PrefilterMaps[0].Texture.get(); }

        const Flourish::Texture* GetBRDFTexture() const;

    private:
        struct CubemapData
        {
            glm::mat4 Proj;
            glm::mat4 View;
            glm::vec4 Params; // [0]: roughness
        };

        struct RenderData
        {
            Ref<Flourish::Framebuffer> Framebuffer;
            Ref<Flourish::Texture> Texture;
            Ref<Flourish::CommandBuffer> CommandBuffer; // One for each face
            Ref<Flourish::ResourceSet> ResourceSet;
        };

    private:
        void Initialize();

    private:
        const bool m_GenerateBRDF = false;

        UUID m_MapAsset;
        bool m_SetsWritten = false;

        RenderData m_EnvironmentMap;
        RenderData m_IrradianceMap;
        RenderData m_BRDFTexture;
        HVector<RenderData> m_PrefilterMaps;

        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::RenderPass> m_BRDFRenderPass;

        Ref<Flourish::Buffer> m_CubemapDataBuffer;
        Ref<Flourish::Buffer> m_BRDFTexBuffer;
        Ref<Flourish::Buffer> m_FrameDataBuffer;

        Ref<Flourish::RenderGraph> m_RenderGraph;
    };
}
