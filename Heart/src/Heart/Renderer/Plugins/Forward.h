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
    struct ForwardCreateInfo
    {
        Ref<Flourish::Texture> OutputTexture;
        Ref<Flourish::Texture> DepthTexture;
        HString8 MeshBatchesPluginName;
        HString8 TextBatchesPluginName;
        HString8 CollectMaterialsPluginName;
        HString8 FrameDataPluginName;
    };

    class Forward : public RenderPlugin
    {
    public:
        Forward(SceneRenderer* renderer, HStringView8 name, const ForwardCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        ForwardCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_EnvMapResourceSet;
        Ref<Flourish::ResourceSet> m_StandardResourceSet;
        Ref<Flourish::ResourceSet> m_TextResourceSet;
        Ref<Flourish::ResourceSet> m_PostProcessResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
    };
}
