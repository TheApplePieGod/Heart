#pragma once

#include "Heart/Core/UUID.h"
#include "Heart/Events/EventEmitter.h"
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
}

namespace Heart
{
    class AppGraphicsInitEvent;
    class AppGraphicsShutdownEvent;
    class EnvironmentMap : public EventListener
    {
    public:
        EnvironmentMap(UUID mapAsset);
        ~EnvironmentMap();

        void Recalculate();
        inline void UpdateMapAsset(UUID asset) { m_MapAsset = asset; }
        inline UUID GetMapAsset() const { return m_MapAsset; }
        inline Flourish::Texture* GetEnvironmentCubemap() { return m_EnvironmentMap.Texture.get(); }
        inline Flourish::Texture* GetIrradianceCubemap() { return m_IrradianceMap.Texture.get(); }
        inline Flourish::Texture* GetPrefilterCubemap() { return m_PrefilterMaps[0].Texture.get(); }
        inline Flourish::Texture* GetBRDFTexture() { return m_BRDFTexture.Texture.get(); }

        void OnEvent(Event& event) override;

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
        };

    private:
        void Initialize();
        void Shutdown();
        bool OnAppGraphicsInit(AppGraphicsInitEvent& event);
        bool OnAppGraphicsShutdown(AppGraphicsShutdownEvent& event);

    private:
        UUID m_MapAsset;
        bool m_Initialized = false;

        RenderData m_EnvironmentMap;
        RenderData m_IrradianceMap;
        RenderData m_BRDFTexture;
        HVector<RenderData> m_PrefilterMaps;

        Ref<Flourish::RenderPass> m_RenderPass;

        Ref<Flourish::Buffer> m_CubemapDataBuffer;
        Ref<Flourish::Buffer> m_FrameDataBuffer;
    };
}