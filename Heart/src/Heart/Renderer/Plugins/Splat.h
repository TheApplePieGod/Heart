#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "glm/glm.hpp"

namespace Flourish
{
    class Texture;
    class RenderPass;
    class ComputePipeline;
    class Framebuffer;
    class ResourceSet;
}

namespace Heart::RenderPlugins
{
    struct SplatCreateInfo
    {
        HString8 FrameDataPluginName;
    };

    class Splat : public RenderPlugin
    {
    public:
        Splat(SceneRenderer* renderer, HStringView8 name, const SplatCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        struct ComputeKeyPushConstants
        {
            glm::mat4 Model;
            u32 ElemCount;
        };

        struct RadixPushConstants
        {
            u32 Shift;
            u32 NumWorkgroups;
            u32 NumBlocksPerWorkgroup;
        };

    private:
        void RebuildGraph(u32 splatCount);

    private:
        SplatCreateInfo m_Info;

        // Sort params
        const u32 m_MaxSplats = 5000000;
        const u32 m_NumBlocksPerWorkgroup = 500;
        const u32 m_WorkgroupSize = 256;
        const u32 m_BinCount = 256;

        u32 m_LastSplatCount = 0;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::ResourceSet> m_RadixResourceSet;
        Ref<Flourish::ResourceSet> m_HistorgramResourceSet;
        Ref<Flourish::ResourceSet> m_KeysResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
        Ref<Flourish::ComputePipeline> m_RadixPipeline;
        Ref<Flourish::ComputePipeline> m_HistogramPipeline;
        Ref<Flourish::ComputePipeline> m_KeysPipeline;
        std::array<Ref<Flourish::Buffer>, 2> m_SortedKeysBuffers;
        Ref<Flourish::Buffer> m_HistogramBuffer;
        Ref<Flourish::Buffer> m_KeyBuffer;
        Ref<Flourish::Buffer> m_CPUBuffer;
        Ref<Flourish::Buffer> m_BuildDataBuffer;
    };
}
