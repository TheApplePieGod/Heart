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

    };

    class Bloom : public RenderPlugin
    {
    public:
        struct BloomData
        {
            glm::vec2 SrcResolution;
            glm::vec2 DstResolution;
            float Threshold;
            float Knee;
            float SampleScale;
            u32 Prefilter;
            glm::vec4 Padding1;
            glm::vec4 Padding2;
        };

    public:
        Bloom(SceneRenderer2* renderer, HStringView8 name, const BloomCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        BloomCreateInfo m_Info;

        Ref<Flourish::Buffer> m_DataBuffer;
        Ref<Flourish::ComputePipeline> m_UpsamplePipeline;
        Ref<Flourish::ComputePipeline> m_DownsamplePipeline;
        Ref<Flourish::Texture> m_DownsampleTexture;
        Ref<Flourish::Texture> m_UpsampleTexture;
        Ref<Flourish::ResourceSet> m_UpsampleResourceSet;
        Ref<Flourish::ResourceSet> m_DownsampleResourceSet;
        Ref<Flourish::ResourceSet> m_CompositeResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;

        u32 m_MipCount;
    };
}
