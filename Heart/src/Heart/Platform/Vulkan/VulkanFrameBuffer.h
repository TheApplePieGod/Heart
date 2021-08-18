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

    private:
        VkFramebuffer m_FrameBuffer;
        VkRenderPass m_RenderPass;
        VkImage m_ColorImage;
        VkImage m_ResolveImage;
        VkImage m_DepthImage;
        VkDeviceMemory m_ColorImageMemory;
        VkDeviceMemory m_ResolveImageMemory;
        VkDeviceMemory m_DepthImageMemory;
        VkImageView m_ColorImageView;
        VkImageView m_ResolveImageView;
        VkImageView m_DepthImageView;
    };
}