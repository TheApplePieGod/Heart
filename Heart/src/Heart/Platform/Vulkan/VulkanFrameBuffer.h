#pragma once

#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanFramebuffer : public Framebuffer
    {
    public:
        VulkanFramebuffer(const FramebufferCreateInfo& createInfo);
        ~VulkanFramebuffer() override;

        void Bind() override;
        void Submit(GraphicsContext& context) override;
        void BindPipeline(const std::string& name) override;

        void* GetRawAttachmentImageHandle(u32 attachmentIndex, FramebufferAttachmentType type) override;

        inline VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }
        inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
        VkCommandBuffer GetCommandBuffer();

    protected:
        Ref<GraphicsPipeline> InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) override;

    private:
        struct VulkanFramebufferAttachment
        {
            VkFormat ColorFormat;
            VkFormat DepthFormat;
            VkImage ColorImage;
            VkImage ResolveImage;
            VkImage DepthImage;
            VkDeviceMemory ColorImageMemory;
            VkDeviceMemory ResolveImageMemory;
            VkDeviceMemory DepthImageMemory;
            VkImageView ColorImageView;
            VkImageView ResolveImageView;
            VkImageView DepthImageView;
            void* ColorImageImGuiId;
            void* ResolveImageImGuiId;
            void* DepthImageImGuiId;
            bool HasResolve;
            bool HasDepth;
        };

    private:
        void AllocateCommandBuffers();
        void FreeCommandBuffers();

        void CreateAttachmentImages(VulkanFramebufferAttachment& attachmentData, VkFormat colorFormat, VkFormat depthFormat);
        void CleanupAttachmentImages(VulkanFramebufferAttachment& attachmentData);

        void CreateFramebuffer();
        void CleanupFramebuffer();

        void Recreate();

    private:
        VkFramebuffer m_Framebuffer;
        VkRenderPass m_RenderPass;
        std::vector<VkCommandBuffer> m_CommandBuffers; // one for each swapchainimage
        std::vector<VulkanFramebufferAttachment> m_AttachmentData;
        std::vector<VkClearValue> m_CachedClearValues;
        std::vector<VkImageView> m_CachedImageViews;
    };
}