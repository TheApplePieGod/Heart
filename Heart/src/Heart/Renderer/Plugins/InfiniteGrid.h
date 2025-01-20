#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class RenderPass;
    class Framebuffer;
    class ResourceSet;
}

namespace Heart::RenderPlugins
{
    struct InfiniteGridCreateInfo
    {
        Ref<Flourish::Texture> OutputColorTexture;
        Ref<Flourish::Texture> OutputDepthTexture;
        bool ClearColorOutput;
        bool ClearDepthOutput;
        HString8 FrameDataPluginName;
    };

    class InfiniteGrid : public RenderPlugin
    {
    public:
        InfiniteGrid(SceneRenderer* renderer, HStringView8 name, const InfiniteGridCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        InfiniteGridCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
    };
}
