#pragma once

#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanSwapChain
    {
    public:
        struct SwapChainData
        {
            VkFormat ImageFormat;
            VkExtent2D Extent;
            std::vector<VkImage> Images;
            std::vector<VkImageView> ImageViews;
            std::vector<VkFramebuffer> FrameBuffers;
        };

    public:
        void Initialize(int width, int height, VkSurfaceKHR surface);
        void Shutdown();
        void RecreateSwapChain();

        inline u32 GetImageCount() const { return static_cast<u32>(m_SwapChainData.Images.size()); }

    private:
        void CreateSwapChain();
        void CleanupSwapChain();
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    private:
        bool m_Initialized = false;
        int m_InitialWidth, m_InitialHeight;
        VkSurfaceKHR m_Surface;
        VkSwapchainKHR m_SwapChain;
        SwapChainData m_SwapChainData;
    };
}