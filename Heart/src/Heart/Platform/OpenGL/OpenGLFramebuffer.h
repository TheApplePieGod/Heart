#pragma once

#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Container/HString8.h"
#include "Heart/Container/HVector.hpp"

namespace Heart
{
    class OpenGLBuffer;
    class OpenGLTexture;
    class OpenGLGraphicsPipeline;
    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        OpenGLFramebuffer(const FramebufferCreateInfo& createInfo);
        ~OpenGLFramebuffer() override;

        void Bind(ComputePipeline* preRenderComputePipeline = nullptr) override;
        void BindPipeline(const HStringView8& name) override;
        void BindShaderBufferResource(u32 bindingIndex, u32 offset, u32 elementCount, Buffer* buffer) override;
        void BindShaderTextureResource(u32 bindingIndex, Texture* texture) override;
        void BindShaderTextureLayerResource(u32 bindingIndex, Texture* texture, u32 layerIndex, u32 mipLevel) override;
        void BindSubpassInputAttachment(u32 bindingIndex, SubpassAttachment attachment) override;
        void FlushBindings() override;

        void* GetColorAttachmentImGuiHandle(u32 attachmentIndex) override;
        void* GetColorAttachmentPixelData(u32 attachmentIndex) override;
        void UpdateColorAttachmentCPUVisibliity(u32 attachmentIndex, bool visible) override;

        double GetPerformanceTimestamp() override;
        double GetSubpassPerformanceTimestamp(u32 subpassIndex) override;

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
            bool AllowCPURead;
            bool CPUVisible;
            bool IsDepthAttachment;
            std::array<Ref<OpenGLBuffer>, 2> PixelBuffers;
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

        void Submit(ComputePipeline* postRenderComputePipeline);
        void Recreate();
        void BlitFramebuffers(int subpassIndex);

    private:
        u32 m_PBOFramebuffer;
        HVector<u32> m_Framebuffers;
        HVector<u32> m_BlitFramebuffers;
        HVector<OpenGLFramebufferAttachment> m_AttachmentData;
        HVector<OpenGLFramebufferAttachment> m_DepthAttachmentData;
        HVector<u32> m_CachedAttachmentHandles;

        std::array<HVector<u32>, 2> m_QueryIds;
        HVector<double> m_PerformanceTimestamps;

        int m_ImageSamples = 1;
        int m_CurrentSubpass = -1;
        bool m_FlushedThisFrame = false;
        OpenGLGraphicsPipeline* m_BoundPipeline = nullptr;
        HString8 m_BoundPipelineName = "";

        friend class OpenGLRenderApi;
    };
}