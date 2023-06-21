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
    struct RTXCreateInfo
    {
        HString8 TLASPluginName;
    };

    class RTX : public RenderPlugin
    {
    public:
        RTX(SceneRenderer* renderer, HStringView8 name, const RTXCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        RTXCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::Texture> m_Output;
        Ref<Flourish::RayTracingPipeline> m_Pipeline;
        Ref<Flourish::RayTracingGroupTable> m_GroupTable;
    };
}
