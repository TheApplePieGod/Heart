#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

namespace Flourish
{
    class ResourceSet;
    class ComputePipeline;
    class Texture;
}

namespace Heart::RenderPlugins
{
    struct SVGFCreateInfo
    {
        Ref<Flourish::Texture> InputTexture;
        Ref<Flourish::Texture> OutputTexture;
        HString8 FrameDataPluginName;
        HString8 GBufferPluginName;

        f32 InputUsableFactorWidth = 1.f;
        f32 InputUsableFactorHeight = 1.f;
    };

    class SVGF : public RenderPlugin
    {
    public:
        SVGF(SceneRenderer* renderer, HStringView8 name, const SVGFCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

        u32 GetArrayIndex() const;
        u32 GetPrevArrayIndex() const;

        inline Flourish::Texture* GetDebugTexture() const { return m_DebugTexture.get(); }

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        struct TemporalPushData
        {
            u32 ShouldReset = 0;
        };

        struct ATrousPushData
        {
            u32 Iteration = 0;
        };

    private:
        SVGFCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_TemporalResourceSet;
        Ref<Flourish::ResourceSet> m_ATrousResourceSet;
        Ref<Flourish::ResourceSet> m_UpsampleResourceSet;
        Ref<Flourish::ComputePipeline> m_TemporalPipeline;
        Ref<Flourish::ComputePipeline> m_ATrousPipeline;
        Ref<Flourish::ComputePipeline> m_UpsamplePipeline;
        Ref<Flourish::Texture> m_ColorHistory;
        Ref<Flourish::Texture> m_MomentsHistory;
        Ref<Flourish::Texture> m_TempTexture;
        Ref<Flourish::Buffer> m_TileData;

        Ref<Flourish::Texture> m_DebugTexture;

        u32 m_ATrousIterations = 4;
        u32 m_GBufferMip = 0;
        TemporalPushData m_TemporalPushData;
        ATrousPushData m_ATrousPushData;
    };
}
