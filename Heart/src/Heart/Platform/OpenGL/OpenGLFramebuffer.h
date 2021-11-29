#pragma once

#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Platform/OpenGL/OpenGLBuffer.h"
#include "Heart/Platform/OpenGL/OpenGLTexture.h"
#include "Heart/Platform/OpenGL/OpenGLGraphicsPipeline.h"

namespace Heart
{
    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        OpenGLFramebuffer(const FramebufferCreateInfo& createInfo);
        ~OpenGLFramebuffer() override;

        void Bind() override;
        void BindPipeline(const std::string& name) override;
        void BindShaderBufferResource(u32 bindingIndex, u32 offset, u32 elementCount, Buffer* buffer) override;
        void BindShaderTextureResource(u32 bindingIndex, Texture* texture) override;
        void BindShaderTextureLayerResource(u32 bindingIndex, Texture* texture, u32 layerIndex) override;
        void BindSubpassInputAttachment(u32 bindingIndex, SubpassAttachment attachment) override;
        void FlushBindings() override;

        void* GetColorAttachmentImGuiHandle(u32 attachmentIndex) override;
        void* GetColorAttachmentPixelData(u32 attachmentIndex) override;

        void ClearOutputAttachment(u32 outputAttachmentIndex, bool clearDepth) override;
        void StartNextSubpass() override;
        
        inline bool CanDraw() const { return m_FlushedThisFrame; }
        inline OpenGLGraphicsPipeline* GetBoundPipeline() { return m_BoundPipeline; }

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
            std::array<Ref<OpenGLBuffer>, 2> PixelBuffers;
            void* PixelBufferMapping = nullptr;
            OpenGLTexture* ExternalTexture;
            u32 ExternalTextureLayer;
            u32 ExternalTextureMip;
            u32 PBOFramebufferAttachment;
        };

    private:
        void CreateAttachmentTextures(OpenGLFramebufferAttachment& attachment);
        void CleanupAttachmentTextures(OpenGLFramebufferAttachment& attachment);

        void PopulatePixelBuffer(OpenGLFramebufferAttachment& attachment);

        void CreateFramebuffers();
        void CleanupFramebuffers();

        void CreatePixelBuffers(OpenGLFramebufferAttachment& attachment);
        void CleanupPixelBuffers(OpenGLFramebufferAttachment& attachment);

        void Submit();
        void Recreate();
        void BlitFramebuffers(int subpassIndex);

    private:
        u32 m_PBOFramebuffer;
        std::vector<u32> m_Framebuffers;
        std::vector<u32> m_BlitFramebuffers;
        std::vector<OpenGLFramebufferAttachment> m_AttachmentData;
        std::vector<OpenGLFramebufferAttachment> m_DepthAttachmentData;
        std::vector<u32> m_CachedAttachmentHandles;

        int m_ImageSamples = 1;
        int m_CurrentSubpass = -1;
        bool m_FlushedThisFrame = false;
        OpenGLGraphicsPipeline* m_BoundPipeline = nullptr;
        std::string m_BoundPipelineName = "";

        friend class OpenGLRenderApi;
    };
}