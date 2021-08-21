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
        inline VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }

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
        VkCommandBuffer m_CommandBuffer; // one for each swapchainimage
        std::vector<VulkanFrameBufferAttachment> m_AttachmentData;
        std::vector<VkClearValue> m_CachedClearValues;
        std::unordered_map<std::string, Ref<GraphicsPipeline>> m_GraphicsPipelines;
    };
}