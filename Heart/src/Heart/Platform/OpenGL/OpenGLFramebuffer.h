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
        void BindShaderInputSet(const ShaderInputBindPoint& bindPoint, u32 setIndex, const std::vector<u32>& bufferOffsets) override;
        void Submit();

        void* GetColorAttachmentImGuiHandle(u32 attachmentIndex) override;
        void* GetDepthAttachmentImGuiHandle() override;

        inline u32 GetFramebufferId() const { return m_FramebufferId; }

    protected:
        Ref<GraphicsPipeline> InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) override;

    private:
        void CreateTextures();

    private:
        u32 m_FramebufferId;
        u32 m_DepthAttachmentTextureId;
        std::vector<u32> m_ColorAttachmentTextureIds;
    };
}