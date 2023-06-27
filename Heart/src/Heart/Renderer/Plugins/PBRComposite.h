#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class ResourceSet;
    class ComputePipeline;
}

namespace Heart::RenderPlugins
{
    struct PBRCompositeCreateInfo
    {
        Ref<Flourish::Texture> InputReflections;
        HString8 FrameDataPluginName;
        HString8 LightingDataPluginName;
        HString8 GBufferPluginName;
        HString8 TLASPluginName;
    };

    class PBRComposite : public RenderPlugin
    {
    public:
        PBRComposite(SceneRenderer* renderer, HStringView8 name, const PBRCompositeCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        PBRCompositeCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::ComputePipeline> m_Pipeline;
    };
}
