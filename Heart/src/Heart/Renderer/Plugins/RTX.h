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
        HString8 FrameDataPluginName;
        HString8 TLASPluginName;
        HString8 LightingDataPluginName;
    };

    class RTX : public RenderPlugin
    {
    public:
        RTX(SceneRenderer* renderer, HStringView8 name, const RTXCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline Flourish::Texture* GetOutputTexture() const { return m_Output.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        RTXCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet0;
        Ref<Flourish::ResourceSet> m_ResourceSet1;
        Ref<Flourish::Texture> m_Output;
        Ref<Flourish::RayTracingPipeline> m_Pipeline;
        Ref<Flourish::RayTracingGroupTable> m_GroupTable;
    };
}
