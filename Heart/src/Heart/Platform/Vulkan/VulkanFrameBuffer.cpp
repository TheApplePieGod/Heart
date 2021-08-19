#include "htpch.h"
#include "VulkanFrameBuffer.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"

namespace Heart
{
    VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferCreateInfo& createInfo)
        : FrameBuffer(createInfo)
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkFormat colorFormat = VulkanCommon::ColorFormatToVulkan(createInfo.ColorFormat);
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
        VkSampleCountFlagBits imageSamples = VulkanCommon::MsaaSampleCountToVulkan(createInfo.SampleCount);
        if (imageSamples > device.MaxMsaaSamples())
            imageSamples = device.MaxMsaaSamples();

        // create the associated images for the framebuffer
        VulkanCommon::CreateImage(
            device.Device(),
            device.PhysicalDevice(),
            createInfo.Width,
            createInfo.Height,
            colorFormat,
            1,
            imageSamples,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_ColorImage,
            m_ColorImageMemory
        );
        VulkanCommon::CreateImage(
            device.Device(),
            device.PhysicalDevice(),
            createInfo.ResolveWidth,
            createInfo.ResolveHeight,
            colorFormat,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_ResolveImage,
            m_ResolveImageMemory
        );
        VulkanCommon::CreateImage(
            device.Device(),
            device.PhysicalDevice(),
            createInfo.Width,
            createInfo.Height,
            depthFormat,
            1,
            imageSamples,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_DepthImage,
            m_DepthImageMemory
        );

        // create associated image views 
        m_ColorImageView = VulkanCommon::CreateImageView(device.Device(), m_ColorImage, colorFormat, 1);
        m_ResolveImageView = VulkanCommon::CreateImageView(device.Device(), m_ResolveImage, colorFormat, 1);
        m_DepthImageView = VulkanCommon::CreateImageView(device.Device(), m_DepthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);

        // create the associated renderpass
        // TODO: adjustable depth precision
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = imageSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = colorFormat;
        colorAttachment.samples = imageSamples;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = colorFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 1;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 2;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        if (imageSamples != VK_SAMPLE_COUNT_1_BIT || createInfo.ForceResolve)
            subpass.pResolveAttachments = &colorAttachmentResolveRef;
        if (createInfo.HasDepth)
            subpass.pDepthStencilAttachment = &depthAttachmentRef;

        // TODO: subpass dependencies
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::vector<VkAttachmentDescription> attachments = { colorAttachment };
        if (imageSamples != VK_SAMPLE_COUNT_1_BIT || createInfo.ForceResolve)
            attachments.emplace_back(colorAttachmentResolve);
        if (createInfo.HasDepth)
            attachments.emplace_back(depthAttachment);

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        HE_VULKAN_CHECK_RESULT(vkCreateRenderPass(device.Device(), &renderPassInfo, nullptr, &m_RenderPass));

        // create the final framebuffer
        std::vector<VkImageView> images = { m_ColorImageView };
        if (imageSamples != VK_SAMPLE_COUNT_1_BIT || createInfo.ForceResolve)
            images.emplace_back(m_ResolveImageView);
        if (createInfo.HasDepth)
            images.emplace_back(m_DepthImageView);

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = static_cast<u32>(images.size());
        framebufferInfo.pAttachments = images.data();
        framebufferInfo.width = createInfo.Width;
        framebufferInfo.height = createInfo.Height;
        framebufferInfo.layers = 1;
        
        HE_VULKAN_CHECK_RESULT(vkCreateFramebuffer(device.Device(), &framebufferInfo, nullptr, &m_FrameBuffer));
    }

    VulkanFrameBuffer::~VulkanFrameBuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkDestroyFramebuffer(device.Device(), m_FrameBuffer, nullptr);

        vkDestroyImageView(device.Device(), m_ColorImageView, nullptr);
        vkDestroyImageView(device.Device(), m_ResolveImageView, nullptr);
        vkDestroyImageView(device.Device(), m_DepthImageView, nullptr);

        vkDestroyImage(device.Device(), m_ColorImage, nullptr);
        vkDestroyImage(device.Device(), m_ResolveImage, nullptr);
        vkDestroyImage(device.Device(), m_DepthImage, nullptr);

        vkFreeMemory(device.Device(), m_ColorImageMemory, nullptr);
        vkFreeMemory(device.Device(), m_ResolveImageMemory, nullptr);
        vkFreeMemory(device.Device(), m_DepthImageMemory, nullptr);

        vkDestroyRenderPass(device.Device(), m_RenderPass, nullptr);
    }
}
