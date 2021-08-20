#include "htpch.h"
#include "VulkanFrameBuffer.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"

namespace Heart
{
    VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferCreateInfo& createInfo)
        : FrameBuffer(createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.Attachments.size() > 0, "Cannot create a framebuffer with zero attachments");

        VulkanDevice& device = VulkanContext::GetDevice();

        // initialize the main command buffer for this framebuffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VulkanContext::GetGraphicsPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        HE_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device.Device(), &allocInfo, &m_CommandBuffer));

        u32 attachmentIndex = 0;
        std::vector<VkAttachmentReference> colorAttachmentRefs = {};
        std::vector<VkAttachmentReference> resolveAttachmentRefs = {};
        std::vector<VkAttachmentReference> depthAttachmentRefs = {};
        std::vector<VkAttachmentDescription> attachmentDescriptions = {};
        std::vector<VkImageView> attachmentImageViews = {};
        for (auto& attachment : createInfo.Attachments)
        {
            VulkanFrameBufferAttachment attachmentData = {};

            VkFormat colorFormat = VulkanCommon::ColorFormatToVulkan(attachment.ColorFormat);
            VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
            VkSampleCountFlagBits imageSamples = VulkanCommon::MsaaSampleCountToVulkan(createInfo.SampleCount);
            if (imageSamples > device.MaxMsaaSamples())
                imageSamples = device.MaxMsaaSamples();
            attachmentData.HasResolve = attachment.ForceResolve || imageSamples > VK_SAMPLE_COUNT_1_BIT;
            attachmentData.HasDepth = attachment.HasDepth;

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
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                attachmentData.ColorImage,
                attachmentData.ColorImageMemory
            );
            if (attachmentData.HasResolve)
            {
                VulkanCommon::CreateImage(
                    device.Device(),
                    device.PhysicalDevice(),
                    attachment.ResolveWidth == 0 ? createInfo.Width : attachment.ResolveWidth,
                    attachment.ResolveHeight == 0 ? createInfo.Height : attachment.ResolveHeight,
                    colorFormat,
                    1,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    attachmentData.ResolveImage,
                    attachmentData.ResolveImageMemory
                );
            }
            if (attachmentData.HasDepth)
            {
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
                    attachmentData.DepthImage,
                    attachmentData.DepthImageMemory
                );
            }

            // create associated image views 
            attachmentData.ColorImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.ColorImage, colorFormat, 1);
            attachmentImageViews.emplace_back(attachmentData.ColorImageView);
            if (attachmentData.HasResolve)
            {
                attachmentData.ResolveImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.ResolveImage, colorFormat, 1);
                attachmentImageViews.emplace_back(attachmentData.ResolveImageView);
            }
            if (attachmentData.HasDepth)
            {
                attachmentData.DepthImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.DepthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);
                attachmentImageViews.emplace_back(attachmentData.DepthImageView);
            }

            // create the associated renderpass
            // TODO: adjustable depth precision
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = colorFormat;
            colorAttachment.samples = imageSamples;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentDescriptions.emplace_back(colorAttachment);

            if (attachmentData.HasResolve)
            {
                VkAttachmentDescription colorAttachmentResolve{};
                colorAttachmentResolve.format = colorFormat;
                colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachmentDescriptions.emplace_back(colorAttachmentResolve);
            }

            if (attachmentData.HasDepth)
            {
                VkAttachmentDescription depthAttachment{};
                depthAttachment.format = depthFormat;
                depthAttachment.samples = imageSamples;
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                attachmentDescriptions.emplace_back(depthAttachment);
            }

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = attachmentIndex++;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentRefs.emplace_back(colorAttachmentRef);

            VkAttachmentReference colorAttachmentResolveRef{};
            colorAttachmentResolveRef.attachment = attachmentData.HasResolve ? attachmentIndex++ : VK_ATTACHMENT_UNUSED;
            colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveAttachmentRefs.emplace_back(colorAttachmentResolveRef);

            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = attachmentData.HasDepth ? attachmentIndex++ : VK_ATTACHMENT_UNUSED;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachmentRefs.emplace_back(depthAttachmentRef);

            m_AttachmentData.emplace_back(attachmentData);
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<u32>(colorAttachmentRefs.size());
        subpass.pColorAttachments = colorAttachmentRefs.data();
        subpass.pResolveAttachments = resolveAttachmentRefs.data();
        subpass.pDepthStencilAttachment = depthAttachmentRefs.data();

        // TODO: subpass dependencies
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<u32>(attachmentDescriptions.size());
        renderPassInfo.pAttachments = attachmentDescriptions.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        HE_VULKAN_CHECK_RESULT(vkCreateRenderPass(device.Device(), &renderPassInfo, nullptr, &m_RenderPass));

        // create the final framebuffer
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = static_cast<u32>(attachmentImageViews.size());
        framebufferInfo.pAttachments = attachmentImageViews.data();
        framebufferInfo.width = createInfo.Width;
        framebufferInfo.height = createInfo.Height;
        framebufferInfo.layers = 1;
        
        HE_VULKAN_CHECK_RESULT(vkCreateFramebuffer(device.Device(), &framebufferInfo, nullptr, &m_FrameBuffer));
    }

    VulkanFrameBuffer::~VulkanFrameBuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkDestroyFramebuffer(device.Device(), m_FrameBuffer, nullptr);

        for (auto& attachmentData : m_AttachmentData)
        {
            vkDestroyImageView(device.Device(), attachmentData.ColorImageView, nullptr);
            vkDestroyImage(device.Device(), attachmentData.ColorImage, nullptr);
            vkFreeMemory(device.Device(), attachmentData.ColorImageMemory, nullptr);

            if (attachmentData.HasResolve)
            {
                vkDestroyImageView(device.Device(), attachmentData.ResolveImageView, nullptr);
                vkDestroyImage(device.Device(), attachmentData.ResolveImage, nullptr);
                vkFreeMemory(device.Device(), attachmentData.ResolveImageMemory, nullptr);
            }

            if (attachmentData.HasDepth)
            {
                vkDestroyImageView(device.Device(), attachmentData.DepthImageView, nullptr);
                vkDestroyImage(device.Device(), attachmentData.DepthImage, nullptr);
                vkFreeMemory(device.Device(), attachmentData.DepthImageMemory, nullptr);
            }
        }

        vkDestroyRenderPass(device.Device(), m_RenderPass, nullptr);

        vkFreeCommandBuffers(device.Device(), VulkanContext::GetGraphicsPool(), 1, &m_CommandBuffer);
    }

    void VulkanFrameBuffer::Bind()
    {
        VulkanContext::SetBoundCommandBuffer(m_CommandBuffer);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        HE_VULKAN_CHECK_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_FrameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = VkExtent2D {m_Info.Width, m_Info.Height};

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.0, 0.0, 0.0, 1.f };
        clearValues[1].color = { 0.0, 0.0, 0.0, 1.f };
        renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanFrameBuffer::Submit(GraphicsContext& _context)
    {
        VulkanContext& context = static_cast<VulkanContext&>(_context);

        vkCmdEndRenderPass(m_CommandBuffer);
        
        HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(m_CommandBuffer));

        context.GetSwapChain().SubmitCommandBuffer(m_CommandBuffer);
    }
}
