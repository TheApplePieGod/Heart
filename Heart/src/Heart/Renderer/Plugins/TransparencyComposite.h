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
    struct TransparencyCompositeCreateInfo
    {
        HString8 FrameDataPluginName;
    };

    class TransparencyComposite : public RenderPlugin
    {
    public:
        TransparencyComposite(SceneRenderer* renderer, HStringView8 name, const TransparencyCompositeCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

        inline const Ref<Flourish::Texture> GetAccumTexture() const { return m_AccumTexture; }
        inline const Ref<Flourish::Texture> GetRevealTexture() const { return m_RevealTexture; }

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        TransparencyCompositeCreateInfo m_Info;

        Ref<Flourish::Texture> m_AccumTexture;
        Ref<Flourish::Texture> m_RevealTexture;
        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
    };
}
