#pragma once

#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Platform/OpenGL/OpenGLBuffer.h"

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

        void* GetColorAttachmentImGuiHandle(u32 attachmentIndex) override;
        void* GetDepthAttachmentImGuiHandle(u32 attachmentIndex) override;

        void* GetColorAttachmentPixelData(u32 attachmentIndex) override;
        void* GetDepthAttachmentPixelData(u32 attachmentIndex) override;

        void StartNextSubpass() override;

        inline u32 GetFramebufferId() const { return m_FramebufferId; }

    protected:
        Ref<GraphicsPipeline> InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) override;

    private:
        void CreateTextures(int framebufferId, MsaaSampleCount sampleCount, std::vector<u32>& attachmentArray, u32& depthAttachment);
        void CleanupTextures(std::vector<u32>& attachmentArray, u32& depthAttachment);

        void CreatePixelBuffers();
        void CleanupPixelBuffers();

        void Submit();

        void Recreate();

    private:
        u32 m_FramebufferId;
        u32 m_BlitFramebufferId;
        u32 m_DepthAttachmentTextureId;
        u32 m_BlitDepthAttachmentTextureId;
        std::vector<u32> m_ColorAttachmentTextureIds;
        std::vector<u32> m_BlitColorAttachmentTextureIds;
        std::vector<u32> m_CachedAttachmentHandles;

        std::vector<std::array<Ref<OpenGLBuffer>, 2>> m_PixelBufferObjects;
        std::vector<void*> m_PixelBufferMappings;

        friend class OpenGLRenderApi;
    };
}