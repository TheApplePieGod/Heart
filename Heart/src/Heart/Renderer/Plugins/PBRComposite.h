#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "glm/vec4.hpp"

namespace Flourish
{
    class ResourceSet;
    class ComputePipeline;
}

namespace Heart::RenderPlugins
{
    struct PBRCompositeCreateInfo
    {
        Ref<Flourish::Texture> OutputTexture;
        HString8 FrameDataPluginName;
        HString8 LightingDataPluginName;
        HString8 GBufferPluginName;
        HString8 SSAOPluginName;
        HString8 ClusteredLightingPluginName;
    };

    class PBRComposite : public RenderPlugin
    {
    public:
        PBRComposite(SceneRenderer* renderer, HStringView8 name, const PBRCompositeCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        struct PushConstants
        {
            u32 SSAOEnable; 
        };

    private:
        PBRCompositeCreateInfo m_Info;

        PushConstants m_PushConstants;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::ComputePipeline> m_Pipeline;
    };
}
