#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class CommandBuffer;
    class Texture;
    class RenderPass;
    class Framebuffer;
    class DescriptorSet;
    class Buffer;
}

namespace Heart::RenderPlugins
{
    struct RenderMaterialBatchesCreateInfo
    {
        HString8 MaterialBatchesPluginName;
        HString8 FrameDataPluginName;
        HString8 LightingDataPluginName;
        u32 Width, Height;
        bool CanOutputEntityIds;
    };

    class RenderMaterialBatches : public RenderPlugin
    {
    public:
        RenderMaterialBatches(HStringView8 name, const RenderMaterialBatchesCreateInfo& createInfo)
            : RenderPlugin(name), m_Info(createInfo)
        { Initialize(); }

        void Resize(u32 width, u32 height) override;

        inline Flourish::CommandBuffer* GetCommandBuffer() const { return m_CommandBuffer.get(); }
        inline Flourish::Texture* GetOutputTexture() const { return m_RenderOutputTexture.get(); }
        inline Flourish::Texture* GetEntityIdsTexture() const { return m_EntityIdsTexture.get(); }
        inline Flourish::Texture* GetDepthTexture() const { return m_DepthTexture.get(); }
        inline Flourish::Buffer* GetEntityIdsBuffer() const { return m_EntityIdsBuffer.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer) override;

    private:
        void Initialize();

    private:
        RenderMaterialBatchesCreateInfo m_Info;

        Ref<Flourish::CommandBuffer> m_CommandBuffer;
        Ref<Flourish::DescriptorSet> m_DescriptorSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
        Ref<Flourish::Texture> m_RenderOutputTexture;
        Ref<Flourish::Texture> m_EntityIdsTexture;
        Ref<Flourish::Texture> m_DepthTexture;
        Ref<Flourish::Texture> m_DefaultEnvironmentMap;
        Ref<Flourish::Buffer> m_EntityIdsBuffer;
    };
}
