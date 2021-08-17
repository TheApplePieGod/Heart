#include "htpch.h"
#include "VulkanCommon.h"

namespace Heart
{
    VulkanCommon::SwapChainSupportDetails VulkanCommon::GetSwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);
        
        u32 formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
        }

        u32 presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
        }

        return details;
    }

    VulkanCommon::QueueFamilyIndices VulkanCommon::GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        VulkanCommon::QueueFamilyIndices indices;

        u32 supportedQueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &supportedQueueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> supportedQueueFamilies(supportedQueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &supportedQueueFamilyCount, supportedQueueFamilies.data());

        int i = 0;
        for (const auto& family : supportedQueueFamilies)
        {
            if (indices.IsComplete())
                break;

            if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.GraphicsFamily = i;

            if (family.queueFlags & VK_QUEUE_COMPUTE_BIT)
                indices.ComputeFamily = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
                indices.PresentFamily = i;
            
            i++;
        }

        return indices;
    }
}