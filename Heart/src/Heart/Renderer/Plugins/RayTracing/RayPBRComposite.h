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
        HString8 ReflectionsInputPluginName;
        HString8 FrameDataPluginName;
        HString8 LightingDataPluginName;
        HString8 GBufferPluginName;
        HString8 ClusteredLightingPluginName;
        HString8 TLASPluginName;
        HString8 CollectMaterialsPluginName;
    };

    class RayPBRComposite : public RenderPlugin
    {
    public:
        RayPBRComposite(SceneRenderer* renderer, HStringView8 name, const RayPBRCompositeCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        RayPBRCompositeCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet0;
        Ref<Flourish::ResourceSet> m_ResourceSet1;
        Ref<Flourish::RayTracingPipeline> m_Pipeline;
        Ref<Flourish::RayTracingGroupTable> m_GroupTable;
    };
}
