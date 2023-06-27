#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "Flourish/Api/RayTracing/AccelerationStructure.h"

namespace Flourish
{
    class ResourceSet;
    class Texture;
    class RayTracingPipeline;
    class RayTracingGroupTable;
}

namespace Heart::RenderPlugins
{
    struct RayReflectionsCreateInfo
    {
        HString8 FrameDataPluginName;
        HString8 TLASPluginName;
        HString8 LightingDataPluginName;
        HString8 GBufferPluginName;
        HString8 CollectMaterialsPluginName;
    };

    class RayReflections : public RenderPlugin
    {
    public:
        RayReflections(SceneRenderer* renderer, HStringView8 name, const RayReflectionsCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline const Ref<Flourish::Texture>& GetOutputTexture() const { return m_Output; }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        RayReflectionsCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet0;
        Ref<Flourish::ResourceSet> m_ResourceSet1;
        Ref<Flourish::Texture> m_Output;
        Ref<Flourish::RayTracingPipeline> m_Pipeline;
        Ref<Flourish::RayTracingGroupTable> m_GroupTable;
    };
}
