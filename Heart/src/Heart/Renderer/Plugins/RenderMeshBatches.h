#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class CommandBuffer;
    class Texture;
    class RenderPass;
    class Framebuffer;
    class DescriptorSet;
}

namespace Heart::RenderPlugins
{
    struct RenderMeshBatchesCreateInfo
    {
        bool WriteNormals;
        HString8 MeshBatchesPluginName;
        HString8 FrameDataPluginName;
    };

    class RenderMeshBatches : public RenderPlugin
    {
    public:
        RenderMeshBatches(SceneRenderer2* renderer, HStringView8 name, const RenderMeshBatchesCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline Flourish::CommandBuffer* GetCommandBuffer() const { return m_CommandBuffer.get(); }
        inline Flourish::Texture* GetNormalsTexture() const { return m_NormalsTexture.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        RenderMeshBatchesCreateInfo m_Info;

        Ref<Flourish::CommandBuffer> m_CommandBuffer;
        Ref<Flourish::DescriptorSet> m_DescriptorSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
        Ref<Flourish::Texture> m_NormalsTexture;
    };
}
