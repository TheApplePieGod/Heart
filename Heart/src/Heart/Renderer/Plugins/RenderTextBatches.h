#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class Texture;
    class RenderPass;
    class Framebuffer;
    class ResourceSet;
    class Buffer;
}

namespace Heart::RenderPlugins
{
    struct RenderTextBatchesCreateInfo
    {
        HString8 TextBatchesPluginName;
        HString8 FrameDataPluginName;
        HString8 LightingDataPluginName;
        HString8 EntityIdsPluginName; // Optional
    };

    class RenderTextBatches : public RenderPlugin
    {
    public:
        RenderTextBatches(SceneRenderer* renderer, HStringView8 name, const RenderTextBatchesCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        RenderTextBatchesCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::ResourceSet> m_TextResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
        Ref<Flourish::Buffer> m_DataBuffer;
    };
}
