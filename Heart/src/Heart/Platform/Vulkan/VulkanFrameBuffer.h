#pragma once

#include "Heart/Renderer/FrameBuffer.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanFrameBuffer : public FrameBuffer
    {
    public:
        VulkanFrameBuffer(const FrameBufferCreateInfo& createInfo);
        ~VulkanFrameBuffer() override;

        void Bind() override;
        void Submit(GraphicsContext& context) override;
        void BindPipeline(const std::string& name) override;

        void* GetRawAttachmentImageHandle(u32 attachmentIndex, FrameBufferAttachmentType type) override;

        inline VkFramebuffer GetFrameBuffer() const { return m_FrameBuffer; }
        inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
        VkCommandBuffer GetCommandBuffer();

    protected:
        Ref<GraphicsPipeline> InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) override;

    private:
        struct VulkanFrameBufferAttachment
        {
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
        VkFramebuffer m_FrameBuffer;
        VkRenderPass m_RenderPass;
        std::vector<VkCommandBuffer> m_CommandBuffers; // one for each swapchainimage
        std::vector<VulkanFrameBufferAttachment> m_AttachmentData;
        std::vector<VkClearValue> m_CachedClearValues;
    };
}