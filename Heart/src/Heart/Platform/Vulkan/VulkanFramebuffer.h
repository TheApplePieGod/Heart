#pragma once

#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Platform/Vulkan/VulkanSwapChain.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Platform/Vulkan/VulkanBuffer.h"

namespace Heart
{
    class VulkanFramebuffer : public Framebuffer
    {
    public:
        VulkanFramebuffer(const FramebufferCreateInfo& createInfo);
        ~VulkanFramebuffer() override;

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

        inline VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }
        inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
        inline VkCommandBuffer GetCommandBuffer() { UpdateFrameIndex(); return m_CommandBuffers[m_InFlightFrameIndex]; } ;

    protected:
        Ref<GraphicsPipeline> InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) override;
        void BindShaderResource(u32 bindingIndex, ShaderResourceType resourceType, void* resource, u32 offset); // offset in bytes

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
        std::string m_BoundPipeline;
        std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_CommandBuffers;
        std::vector<VulkanFramebufferAttachment> m_AttachmentData;
        std::vector<VulkanFramebufferAttachment> m_DepthAttachmentData;
        std::vector<VkClearValue> m_CachedClearValues;
        std::vector<VkImageView> m_CachedImageViews;

        u64 m_LastUpdateFrame = 0;
        u32 m_InFlightFrameIndex = 0;
        VkSampleCountFlagBits m_ImageSamples;
        bool m_SubmittedThisFrame = false;
        bool m_BoundThisFrame = false;

        friend class VulkanRenderApi;
        friend class VulkanSwapChain;
        friend class VulkanDescriptorSet;
    };
}