#pragma once

#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanTexture;
    class VulkanBuffer;
    class VulkanGraphicsPipeline;
    class VulkanFramebuffer : public Framebuffer
    {
    public:
        VulkanFramebuffer(const FramebufferCreateInfo& createInfo);
        ~VulkanFramebuffer() override;

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

        inline VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }
        inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
        inline VkCommandBuffer GetCommandBuffer() { UpdateFrameIndex(); return m_CommandBuffers[m_InFlightFrameIndex]; }
        inline bool CanDraw() const { return m_FlushedThisFrame; }
        inline VulkanGraphicsPipeline* GetBoundPipeline() { return m_BoundPipeline; }
        inline void PushAuxiliaryCommandBuffer(VkCommandBuffer buffer) { UpdateFrameIndex(); m_AuxiliaryCommandBuffers[m_InFlightFrameIndex].push_back(buffer); }
        inline bool WasBoundThisFrame() const { return m_BoundThisFrame; }
        inline bool WasSubmittedThisFrame() const { return m_SubmittedThisFrame; }

    protected:
        Ref<GraphicsPipeline> InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) override;
        void BindShaderResource(u32 bindingIndex, ShaderResourceType resourceType, void* resource, bool useOffset, u32 offset, u32 size); // buffer offset: bytes, image offset: layerIndex, buffer size: bytes

    private:
        struct VulkanFramebufferAttachment
        {
            ColorFormat GeneralColorFormat;
            VkFormat ColorFormat;
            VkImage Image;
            VkImage ResolveImage;
            VkDeviceMemory ImageMemory;
            VkDeviceMemory ResolveImageMemory;
            VkImageView ImageView;
            VkImageView ResolveImageView;
            u32 ImageAttachmentIndex;
            u32 ResolveImageAttachmentIndex;
            void* ImageImGuiId;
            void* ResolveImageImGuiId;
            bool HasResolve;
            bool CPUVisible;
            bool IsDepthAttachment;
            VulkanTexture* ExternalTexture;
            u32 ExternalTextureLayer;
            u32 ExternalTextureMip;
            Ref<VulkanBuffer> AttachmentBuffer;
        };

    private:
        void AllocateCommandBuffers();
        void FreeCommandBuffers();

        void CreateAttachmentImages(VulkanFramebufferAttachment& attachmentData);
        void CleanupAttachmentImages(VulkanFramebufferAttachment& attachmentData);

        void CreateFramebuffer();
        void CleanupFramebuffer();

        void Recreate();

        void UpdateFrameIndex();
        void Submit();
        void CopyAttachmentToBuffer(VulkanFramebufferAttachment& attachmentData);

    private:
        VkFramebuffer m_Framebuffer;
        VkRenderPass m_RenderPass;
        VulkanGraphicsPipeline* m_BoundPipeline = nullptr;
        std::string m_BoundPipelineName = "";
        std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_CommandBuffers;
        std::vector<VulkanFramebufferAttachment> m_AttachmentData;
        std::vector<VulkanFramebufferAttachment> m_DepthAttachmentData;
        std::vector<VkClearValue> m_CachedClearValues;
        std::vector<VkImageView> m_CachedImageViews;
        std::array<std::vector<VkCommandBuffer>, MAX_FRAMES_IN_FLIGHT> m_AuxiliaryCommandBuffers;

        u32 m_CurrentSubpass = 0;
        u64 m_LastUpdateFrame = 0;
        u32 m_InFlightFrameIndex = 0;
        VkSampleCountFlagBits m_ImageSamples;
        bool m_SubmittedThisFrame = false;
        bool m_BoundThisFrame = false;
        bool m_FlushedThisFrame = false;

        friend class VulkanRenderApi;
        friend class VulkanSwapChain;
        friend class VulkanDescriptorSet;
    };
}