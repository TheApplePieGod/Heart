#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "glm/vec4.hpp"

namespace Flourish
{
    class ResourceSet;
    class RayTracingPipeline;
    class RayTracingGroupTable;
}

namespace Heart::RenderPlugins
{
    struct RayPBRCompositeCreateInfo
    {
        Ref<Flourish::Texture> ReflectionsInputTexture;
        Ref<Flourish::Texture> OutputTexture;
        HString8 FrameDataPluginName;
        HString8 LightingDataPluginName;
        HString8 GBufferPluginName;
        HString8 SSAOPluginName;
        HString8 ClusteredLightingPluginName;
        HString8 TLASPluginName;
        HString8 CollectMaterialsPluginName;
    };

    class RayPBRComposite : public RenderPlugin
    {
    public:
        RayPBRComposite(SceneRenderer* renderer, HStringView8 name, const RayPBRCompositeCreateInfo& createInfo)
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
        RayPBRCompositeCreateInfo m_Info;

        PushConstants m_PushConstants;

        Ref<Flourish::ResourceSet> m_ResourceSet0;
        Ref<Flourish::ResourceSet> m_ResourceSet1;
        Ref<Flourish::RayTracingPipeline> m_Pipeline;
        Ref<Flourish::RayTracingGroupTable> m_GroupTable;
    };
}
