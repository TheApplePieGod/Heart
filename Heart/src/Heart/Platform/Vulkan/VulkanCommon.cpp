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

    u32 VulkanCommon::FindMemoryType(VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (u32 i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        HT_ENGINE_ASSERT(false, "Failed to find suitable memory type");
        return 0;
    }

    void VulkanCommon::CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, u32 width, u32 height, VkFormat format, u32 mipLevels, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageLayout initialLayout)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<u32>(width);
        imageInfo.extent.height = static_cast<u32>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = initialLayout;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = numSamples;
        imageInfo.flags = 0; // Optional

        HT_VULKAN_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        HT_VULKAN_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    VkImageView VulkanCommon::CreateImageView(VkDevice device, VkImage image, VkFormat format, u32 mipLevels, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView view;
        HT_VULKAN_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &view));

        return view;
    }

    VkFormat VulkanCommon::ColorFormatToVulkan(ColorFormat format)
    {
        // TODO: make these more robust
        switch (format)
        {
            default:
            { HT_ENGINE_ASSERT(false, "Vulkan does not support specified color format"); } break;
            case ColorFormat::R8: return VK_FORMAT_R8_SRGB;
            case ColorFormat::RG8: return VK_FORMAT_R8G8_SRGB;
            case ColorFormat::RGB8: return VK_FORMAT_R8G8B8_SRGB;
            case ColorFormat::RGBA8: return VK_FORMAT_R8G8B8A8_SRGB;
            case ColorFormat::RGBA32: return VK_FORMAT_R32G32B32A32_SFLOAT;
        }

        return VK_FORMAT_R8_SRGB;
    }

    VkSampleCountFlagBits VulkanCommon::MsaaSampleCountToVulkan(MsaaSampleCount sampleCount)
    {
        switch (sampleCount)
        {
            default:
            { HT_ENGINE_ASSERT(false, "Vulkan does not support specified sample count"); } break;
            case MsaaSampleCount::None: return VK_SAMPLE_COUNT_1_BIT;
            case MsaaSampleCount::Two: return VK_SAMPLE_COUNT_2_BIT;
            case MsaaSampleCount::Four: return VK_SAMPLE_COUNT_4_BIT;
            case MsaaSampleCount::Eight: return VK_SAMPLE_COUNT_8_BIT;
            case MsaaSampleCount::Sixteen: return VK_SAMPLE_COUNT_16_BIT;
            case MsaaSampleCount::Thirtytwo: return VK_SAMPLE_COUNT_32_BIT;
            case MsaaSampleCount::Sixtyfour: return VK_SAMPLE_COUNT_64_BIT;
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }
}