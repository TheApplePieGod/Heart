#include "htpch.h"
#include "VulkanSwapChain.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "GLFW/glfw3.h"

namespace Heart
{
    void VulkanSwapChain::Initialize(int width, int height, VkSurfaceKHR surface)
    {
        if (m_Initialized) return;
        m_Surface = surface;
        m_InitialWidth = width;
        m_InitialHeight = height;

        VulkanDevice& device = VulkanContext::GetDevice();

        // allocate the secondary command buffer used for rendering to the swap chain
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VulkanContext::GetGraphicsPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocInfo.commandBufferCount = 1;
        HT_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device.Device(), &allocInfo, &m_CommandBuffer));

        CreateSwapChain();

        CreateRenderPass();

        CreateFrameBufferImages();

        CreateFrameBuffers();

        AllocateCommandBuffers();

        m_Initialized = true;
    }

    void VulkanSwapChain::Shutdown()
    {
        if (!m_Initialized) return;

        VulkanDevice& device = VulkanContext::GetDevice();

        vkFreeCommandBuffers(device.Device(), VulkanContext::GetGraphicsPool(), 1, &m_CommandBuffer);

        FreeCommandBuffers();

        CleanupFrameBuffers();

        CleanupFrameBufferImages();

        CleanupRenderPass();
        
        CleanupSwapChain();

        m_Initialized = false;
    }

    void VulkanSwapChain::CreateSwapChain()
    {
        VkInstance instance = VulkanContext::GetInstance();
        VulkanDevice& device = VulkanContext::GetDevice();

        VulkanCommon::SwapChainSupportDetails swapChainSupport = VulkanCommon::GetSwapChainSupport(device.PhysicalDevice(), m_Surface);
        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

        u32 imageCount = swapChainSupport.Capabilities.minImageCount + 1;
        if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
            imageCount = swapChainSupport.Capabilities.maxImageCount;

        u32 queueIndices[] = { device.GraphicsQueueIndex(), device.PresentQueueIndex() };

        m_SwapChainData.ImageFormat = surfaceFormat.format;
        m_SwapChainData.Extent = extent;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_Surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (device.GraphicsQueueIndex() != device.PresentQueueIndex())
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        HT_VULKAN_CHECK_RESULT(vkCreateSwapchainKHR(device.Device(), &createInfo, nullptr, &m_SwapChain));

        vkGetSwapchainImagesKHR(device.Device(), m_SwapChain, &imageCount, nullptr);
        m_SwapChainData.Images.resize(imageCount);
        vkGetSwapchainImagesKHR(device.Device(), m_SwapChain, &imageCount, m_SwapChainData.Images.data());

        m_SwapChainData.ImageViews.resize(m_SwapChainData.Images.size());
        for (int i = 0; i < m_SwapChainData.Images.size(); i++)
        {
            m_SwapChainData.ImageViews[i] = VulkanCommon::CreateImageView(device.Device(), m_SwapChainData.Images[i], m_SwapChainData.ImageFormat, 1);
        }
    }

    void VulkanSwapChain::CleanupSwapChain()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        for (int i = 0; i < m_SwapChainData.ImageViews.size(); i++)
        {
            vkDestroyImageView(device.Device(), m_SwapChainData.ImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(device.Device(), m_SwapChain, nullptr);
    }

    void VulkanSwapChain::CreateRenderPass()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        // similar to VulkanFramebuffer, create the main renderpass designed to render to the swapchain
        // TODO: variable samplecount
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = device.MaxMsaaSamples();
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_SwapChainData.ImageFormat;
        colorAttachment.samples = device.MaxMsaaSamples();
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = m_SwapChainData.ImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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
        subpass.pResolveAttachments = &colorAttachmentResolveRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, colorAttachmentResolve, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        HT_VULKAN_CHECK_RESULT(vkCreateRenderPass(device.Device(), &renderPassInfo, nullptr, &m_RenderPass));
    }

    void VulkanSwapChain::CleanupRenderPass()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkDestroyRenderPass(device.Device(), m_RenderPass, nullptr);
    }

    void VulkanSwapChain::CreateFrameBufferImages()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkFormat colorFormat = m_SwapChainData.ImageFormat;
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

        // TODO: variable samplecount
        VulkanCommon::CreateImage(
            device.Device(),
            device.PhysicalDevice(),
            m_SwapChainData.Extent.width,
            m_SwapChainData.Extent.height,
            colorFormat,
            1,
            device.MaxMsaaSamples(), 
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_ColorImage,
            m_ColorImageMemory
        );
        m_ColorImageView = VulkanCommon::CreateImageView(device.Device(), m_ColorImage, colorFormat, 1);

        VulkanCommon::CreateImage(
            device.Device(),
            device.PhysicalDevice(),
            m_SwapChainData.Extent.width,
            m_SwapChainData.Extent.height,
            depthFormat,
            1,
            device.MaxMsaaSamples(),
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_DepthImage,
            m_DepthImageMemory
        );
        m_DepthImageView = VulkanCommon::CreateImageView(device.Device(), m_DepthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);      
    }

    void VulkanSwapChain::CleanupFrameBufferImages()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkDestroyImageView(device.Device(), m_ColorImageView, nullptr);
        vkDestroyImageView(device.Device(), m_DepthImageView, nullptr);

        vkDestroyImage(device.Device(), m_ColorImage, nullptr);
        vkDestroyImage(device.Device(), m_DepthImage, nullptr);

        vkFreeMemory(device.Device(), m_ColorImageMemory, nullptr);
        vkFreeMemory(device.Device(), m_DepthImageMemory, nullptr);
    }

    void VulkanSwapChain::CreateFrameBuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        m_SwapChainData.FrameBuffers.resize(m_SwapChainData.ImageViews.size());
        for (int i = 0; i < m_SwapChainData.ImageViews.size(); i++)
        {
            std::array<VkImageView, 3> attachments = { m_ColorImageView, m_SwapChainData.ImageViews[i], m_DepthImageView };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_SwapChainData.Extent.width;
            framebufferInfo.height = m_SwapChainData.Extent.height;
            framebufferInfo.layers = 1;
            
            HT_VULKAN_CHECK_RESULT(vkCreateFramebuffer(device.Device(), &framebufferInfo, nullptr, &m_SwapChainData.FrameBuffers[i]));
        }
    }

    void VulkanSwapChain::CleanupFrameBuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        for (int i = 0; i < m_SwapChainData.FrameBuffers.size(); i++)
        {
            vkDestroyFramebuffer(device.Device(), m_SwapChainData.FrameBuffers[i], nullptr);
        }
    }

    void VulkanSwapChain::AllocateCommandBuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        m_SwapChainData.CommandBuffers.resize(m_SwapChainData.FrameBuffers.size());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VulkanContext::GetGraphicsPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<u32>(m_SwapChainData.CommandBuffers.size());

        HT_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device.Device(), &allocInfo, m_SwapChainData.CommandBuffers.data()));
    }

    void VulkanSwapChain::FreeCommandBuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkFreeCommandBuffers(device.Device(), VulkanContext::GetGraphicsPool(), static_cast<u32>(m_SwapChainData.CommandBuffers.size()), m_SwapChainData.CommandBuffers.data());
    }

    void VulkanSwapChain::RecreateSwapChain()
    {
        FreeCommandBuffers();

        CleanupFrameBuffers();

        CleanupFrameBufferImages();

        CleanupSwapChain();

        CreateSwapChain();

        CreateFrameBufferImages();

        CreateFrameBuffers();

        AllocateCommandBuffers();
    }

    VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
    {
        for (const auto& availableFormat : formats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return availableFormat;
        }

        return formats[0];
    }

    VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
    {
        for (const auto& availablePresentMode : presentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return availablePresentMode;
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
            return capabilities.currentExtent;
        else
        {
            VkExtent2D actualExtent = {
                static_cast<u32>(m_InitialWidth),
                static_cast<u32>(m_InitialWidth)
            };

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }
}