#pragma once

#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Platform/Vulkan/VulkanSwapChain.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanFramebuffer : public Framebuffer
    {
    public:
        VulkanFramebuffer(const FramebufferCreateInfo& createInfo);
        ~VulkanFramebuffer() override;

        void Bind() override;
        void BindPipeline(const std::string& name) override;
        void BindShaderInputSet(const ShaderInputBindPoint& bindPoint, u32 setIndex, const std::vector<u32>& bufferOffsets) override;
        void Submit(GraphicsContext& context) override;

        void* GetColorAttachmentImGuiHandle(u32 attachmentIndex) override;
        void* GetDepthAttachmentImGuiHandle() override;

        inline VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }
        inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
        inline VkCommandBuffer GetCommandBuffer() { UpdateFrameIndex(); return m_CommandBuffers[m_InFlightFrameIndex]; } ;

    protected:
        Ref<GraphicsPipeline> InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) override;

    private:
        struct VulkanFramebufferAttachment
        {
            VkFormat ColorFormat;
            VkImage ColorImage;
            VkImage ResolveImage;
            VkDeviceMemory ColorImageMemory;
            VkDeviceMemory ResolveImageMemory;
            VkImageView ColorImageView;
            VkImageView ResolveImageView;
            void* ColorImageImGuiId;
            void* ResolveImageImGuiId;
            bool HasResolve;
        };

    private:
        void AllocateCommandBuffers();
        void FreeCommandBuffers();

        void CreateAttachmentImages(VulkanFramebufferAttachment& attachmentData, VkFormat colorFormat);
        void CleanupAttachmentImages(VulkanFramebufferAttachment& attachmentData);

        void CreateDepthAttachment();
        void CleanupDepthAttachment();

        void CreateFramebuffer();
        void CleanupFramebuffer();

        void Recreate();

        void UpdateFrameIndex();

    private:
        VkFramebuffer m_Framebuffer;
        VkRenderPass m_RenderPass;
        std::string m_BoundPipeline;
        std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_CommandBuffers;
        std::vector<VulkanFramebufferAttachment> m_AttachmentData;
        std::vector<VkClearValue> m_CachedClearValues;
        std::vector<VkImageView> m_CachedImageViews;

        u64 m_LastUpdateFrame = 0;
        u32 m_InFlightFrameIndex = 0;
        VkSampleCountFlagBits m_ImageSamples;

        VkImage m_DepthImage;
        VkDeviceMemory m_DepthImageMemory;
        VkImageView m_DepthImageView;
        void* m_DepthImageImGuiId;
        VkFormat m_DepthFormat;
    };
}