#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

namespace Flourish
{
    class ResourceSet;
    class ComputePipeline;
    class Texture;
    class Buffer;
    class RenderPass;
    class Framebuffer;
}

namespace Heart::RenderPlugins
{
    struct BloomCreateInfo
    {
        Ref<Flourish::Texture> InputTexture;
        Ref<Flourish::Texture> OutputTexture;
    };

    class Bloom : public RenderPlugin
    {
    public:
        Bloom(SceneRenderer* renderer, HStringView8 name, const BloomCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        struct SamplePushData
        {
            glm::vec2 SrcResolution;
            glm::vec2 DstResolution;
            float Threshold;
            float Knee;
            float SampleScale;
            u32 Prefilter;
        };

        struct CompositePushData
        {
            glm::vec2 SrcResolution;
            glm::vec2 DstResolution;
            float Strength;
        };

    private:
        BloomCreateInfo m_Info;

        Ref<Flourish::ComputePipeline> m_UpsamplePipeline;
        Ref<Flourish::ComputePipeline> m_DownsamplePipeline;
        Ref<Flourish::ComputePipeline> m_CompositePipeline;
        Ref<Flourish::Texture> m_DownsampleTexture;
        Ref<Flourish::Texture> m_UpsampleTexture;
        Ref<Flourish::ResourceSet> m_UpsampleResourceSet;
        Ref<Flourish::ResourceSet> m_DownsampleResourceSet;
        Ref<Flourish::ResourceSet> m_CompositeResourceSet;

        u32 m_MipCount;
        SamplePushData m_PushData;
    };
}
