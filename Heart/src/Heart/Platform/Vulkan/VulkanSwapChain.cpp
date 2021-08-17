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
        CreateSwapChain();

        m_Initialized = true;
    }

    void VulkanSwapChain::Shutdown()
    {
        if (!m_Initialized) return;

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

        for (int i = 0; i < m_SwapChainData.FrameBuffers.size(); i++)
        {
            vkDestroyFramebuffer(device.Device(), m_SwapChainData.FrameBuffers[i], nullptr);
        }

        for (int i = 0; i < m_SwapChainData.ImageViews.size(); i++)
        {
            vkDestroyImageView(device.Device(), m_SwapChainData.ImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(device.Device(), m_SwapChain, nullptr);
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