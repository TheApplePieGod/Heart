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
            std::vector<VkCommandBuffer> CommandBuffers;
        };

    public:
        void Initialize(int width, int height, VkSurfaceKHR surface);
        void Shutdown();

        void BeginFrame();
        void EndFrame();

        void InvalidateSwapChain(u32 newWidth, u32 newHeight);

        inline u32 GetImageCount() const { return static_cast<u32>(m_SwapChainData.Images.size()); }
        inline VkFormat GetImageFormat() const { return m_SwapChainData.ImageFormat; }
        inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
        inline VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffers[m_PresentImageIndex]; }

    private:
        void CreateSwapChain();
        void CleanupSwapChain();
        void RecreateSwapChain();

        void CreateRenderPass();
        void CleanupRenderPass();

        void CreateFrameBufferImages();
        void CleanupFrameBufferImages();

        void CreateFrameBuffers();
        void CleanupFrameBuffers();

        void AllocateCommandBuffers();
        void FreeCommandBuffers();

        void CreateSynchronizationObjects();
        void CleanupSynchronizationObjects();

        void Present();

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    private:
        // TODO: make this a parameter?
        const u32 m_MaxFramesInFlight = 2;
        glm::vec4 m_ClearColor = { 0.f, 0.f, 0.f, 1.f };

        bool m_Initialized = false;
        int m_Width, m_Height;
        VkSurfaceKHR m_Surface;
        VkSwapchainKHR m_SwapChain;
        SwapChainData m_SwapChainData;
        VkRenderPass m_RenderPass;
        std::vector<VkCommandBuffer> m_CommandBuffers = {}; // secondary
        std::vector<VkSemaphore> m_ImageAvailableSemaphores = {};
        std::vector<VkSemaphore> m_RenderFinishedSemaphores = {};
        std::vector<VkFence> m_InFlightFences = {};
        std::vector<VkFence> m_ImagesInFlight = {};
        u32 m_PresentImageIndex;
        bool m_ShouldPresentThisFrame;
        u32 m_InFlightFrameIndex = 0;
        bool m_SwapChainInvalid = false;

        VkImage m_ColorImage;
        VkImage m_DepthImage;

        VkImageView m_ColorImageView;
        VkImageView m_DepthImageView;

        VkDeviceMemory m_ColorImageMemory;
        VkDeviceMemory m_DepthImageMemory;
    };
}