#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class Texture;
    class RenderPass;
    class Framebuffer;
    class ResourceSet;
}

namespace Heart::RenderPlugins
{
    struct ColorGradingCreateInfo
    {

    };

    class ColorGrading : public RenderPlugin
    {
    public:
        ColorGrading(SceneRenderer* renderer, HStringView8 name, const ColorGradingCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        ColorGradingCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
    };
}
