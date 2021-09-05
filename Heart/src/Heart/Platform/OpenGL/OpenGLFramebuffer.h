#pragma once

#include "Heart/Renderer/Framebuffer.h"

namespace Heart
{
    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        OpenGLFramebuffer(const FramebufferCreateInfo& createInfo);
        ~OpenGLFramebuffer() override;

        void Bind() override;
        void BindPipeline(const std::string& name) override;
        void BindShaderBufferResource(u32 bindingIndex, u32 offset, Buffer* buffer) override;
        void BindShaderTextureResource(u32 bindingIndex, Texture* texture) override;
        void Submit();

        void* GetColorAttachmentImGuiHandle(u32 attachmentIndex) override;
        void* GetDepthAttachmentImGuiHandle() override;

        inline u32 GetFramebufferId() const { return m_FramebufferId; }

    protected:
        Ref<GraphicsPipeline> InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) override;

    private:
        void CreateTextures();
        void CleanupTextures();

        void Recreate();

    private:
        u32 m_FramebufferId;
        u32 m_DepthAttachmentTextureId;
        std::vector<u32> m_ColorAttachmentTextureIds;
    };
}