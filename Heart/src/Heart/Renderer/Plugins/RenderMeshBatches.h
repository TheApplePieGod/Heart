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
        u32 Width, Height;
    };

    class RenderMeshBatches : public RenderPlugin
    {
    public:
        RenderMeshBatches(HStringView8 name, const RenderMeshBatchesCreateInfo& createInfo)
            : RenderPlugin(name), m_Info(createInfo)
        { Initialize(); }

        void Resize(u32 width, u32 height) override;

        inline Flourish::CommandBuffer* GetCommandBuffer() const { return m_CommandBuffer.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer) override;

    private:
        void Initialize();

    private:
        RenderMeshBatchesCreateInfo m_Info;

        Ref<Flourish::CommandBuffer> m_CommandBuffer;
        Ref<Flourish::DescriptorSet> m_DescriptorSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
        Ref<Flourish::Texture> m_NormalsTexture;
        Ref<Flourish::Texture> m_DepthTexture;
    };
}
