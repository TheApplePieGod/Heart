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
    struct RenderEnvironmentMapCreateInfo
    {
        Ref<Flourish::Texture> OutputTexture;
        bool ClearOutput;
        HString8 FrameDataPluginName;
    };

    class RenderEnvironmentMap : public RenderPlugin
    {
    public:
        RenderEnvironmentMap (SceneRenderer* renderer, HStringView8 name, const RenderEnvironmentMapCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        RenderEnvironmentMapCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
    };
}
