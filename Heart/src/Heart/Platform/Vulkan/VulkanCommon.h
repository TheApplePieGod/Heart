#pragma once

#include "vulkan/vulkan.h"
#include "Heart/Renderer/FrameBuffer.h"

namespace Heart
{
    struct VulkanCommon
    {
        struct QueueFamilyIndices
        {
            std::optional<u32> GraphicsFamily;
            std::optional<u32> PresentFamily;
            std::optional<u32> ComputeFamily;
            
            bool IsComplete()
            {
                return (
                    GraphicsFamily.has_value() &&
                    PresentFamily.has_value() &&
                    ComputeFamily.has_value()
                );
            }
        };

        struct SwapChainSupportDetails
        {
            VkSurfaceCapabilitiesKHR Capabilities;
            std::vector<VkSurfaceFormatKHR> Formats;
            std::vector<VkPresentModeKHR> PresentModes;
        };

        static SwapChainSupportDetails GetSwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
        static QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
        static u32 FindMemoryType(VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags properties);
        static void CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, u32 width, u32 height, VkFormat format, u32 mipLevels, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);
        static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, u32 mipLevels, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
        static VkFormat ColorFormatToVulkan(ColorFormat format);
        static VkSampleCountFlagBits MsaaSampleCountToVulkan(MsaaSampleCount sampleCount);
    };
}

#define HT_VULKAN_CHECK_RESULT(func) { auto result = func; HT_ENGINE_ASSERT(result == 0, "Vulkan function failed with error {1}", result); }