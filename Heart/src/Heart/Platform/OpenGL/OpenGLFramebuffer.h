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
        void BindSubpassInputAttachment(u32 bindingIndex, SubpassAttachment attachment) override;

        void* GetColorAttachmentImGuiHandle(u32 attachmentIndex) override;
        void* GetDepthAttachmentImGuiHandle(u32 attachmentIndex) override;

        void* GetColorAttachmentPixelData(u32 attachmentIndex) override;
        void* GetDepthAttachmentPixelData(u32 attachmentIndex) override;

        void ClearOutputAttachment(u32 outputAttachmentIndex, bool clearDepth) override;
        void StartNextSubpass() override;

    protected:
        Ref<GraphicsPipeline> InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) override;

    private:
        struct OpenGLFramebufferAttachment
        {
            ColorFormat GeneralColorFormat;
            u32 ColorFormat;
            u32 ColorFormatInternal;
            u32 Image;
            u32 BlitImage;
            bool HasResolve;
            bool CPUVisible;
            bool IsDepthAttachment;
        };

    private:
        void CreateAttachmentTextures(OpenGLFramebufferAttachment& attachment);
        void CleanupAttachmentTextures(OpenGLFramebufferAttachment& attachment);

        void CreateFramebuffers();
        void CleanupFramebuffers();

        void CreatePixelBuffers();
        void CleanupPixelBuffers();

        void Submit();
        void Recreate();
        void BlitFramebuffers(int subpassIndex);

    private:
        std::vector<u32> m_Framebuffers;
        std::vector<u32> m_BlitFramebuffers;
        std::vector<OpenGLFramebufferAttachment> m_AttachmentData;
        std::vector<OpenGLFramebufferAttachment> m_DepthAttachmentData;
        std::vector<u32> m_CachedAttachmentHandles;

        std::vector<std::array<Ref<OpenGLBuffer>, 2>> m_PixelBufferObjects;
        std::vector<void*> m_PixelBufferMappings;

        int m_ImageSamples = 1;
        int m_CurrentSubpass = -1;

        friend class OpenGLRenderApi;
    };
}