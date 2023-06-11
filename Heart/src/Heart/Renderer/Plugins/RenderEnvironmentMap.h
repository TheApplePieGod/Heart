#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class CommandBuffer;
    class Texture;
    class RenderPass;
    class Framebuffer;
    class ResourceSet;
}

namespace Heart::RenderPlugins
{
    struct RenderEnvironmentMapCreateInfo
    {
        HString8 FrameDataPluginName;
    };

    class RenderEnvironmentMap : public RenderPlugin
    {
    public:
        RenderEnvironmentMap (SceneRenderer2* renderer, HStringView8 name, const RenderEnvironmentMapCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline Flourish::CommandBuffer* GetCommandBuffer() const { return m_CommandBuffer.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        RenderEnvironmentMapCreateInfo m_Info;

        Ref<Flourish::CommandBuffer> m_CommandBuffer;
        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
    };
}
