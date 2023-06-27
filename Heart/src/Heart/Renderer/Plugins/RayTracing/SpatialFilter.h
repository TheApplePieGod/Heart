#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

namespace Flourish
{
    class ResourceSet;
    class ComputePipeline;
    class Texture;
}

namespace Heart::RenderPlugins
{
    struct SpatialFilterCreateInfo
    {
        Ref<Flourish::Texture> InputTexture;
    };

    class SpatialFilter : public RenderPlugin
    {
    public:
        SpatialFilter(SceneRenderer* renderer, HStringView8 name, const SpatialFilterCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline Flourish::Texture* GetOutput() const { return m_Output.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        SpatialFilterCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::ComputePipeline> m_Pipeline;
        Ref<Flourish::Texture> m_Output;

        u32 m_MipCount;
    };
}
