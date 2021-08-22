#pragma once

#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    // TODO: make this a parameter?
    const u32 MAX_FRAMES_IN_FLIGHT = 2;

    class FrameDataRegistry
    {
    public:
        std::array<void*, MAX_FRAMES_IN_FLIGHT>& RegisterData(u32& outId);
        void UnregisterData(u32 id);
        inline void* GetCurrentData(u32 id, u32 frameIndex) { return m_FrameDataRegistry[id][frameIndex]; }; // disable checks here for speed

    private:
        std::unordered_map<u32, std::array<void*, MAX_FRAMES_IN_FLIGHT>> m_FrameDataRegistry;
        u32 m_CurrentId = 0;
    };

    class VulkanSwapChain
    {
    public:
        struct SwapChainData
        {
            VkFormat ImageFormat;
            VkExtent2D Extent;
            std::vector<VkImage> Images;
            std::vector<VkImageView> ImageViews;
            std::vector<VkFramebuffer> Framebuffers;
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
        inline VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffers[m_InFlightFrameIndex]; }
        inline void SubmitCommandBuffer(VkCommandBuffer buffer) { m_AuxiliaryCommandBuffers.emplace_back(buffer); }
        inline u32 GetPresentImageIndex() const { return m_PresentImageIndex; }
        inline u32 GetInFlightFrameIndex() const { return m_InFlightFrameIndex; }
        inline FrameDataRegistry& GetFrameDataRegistry() { return m_FrameDataRegistry; }
        inline void* GetFrameData(u32 registryId) { return m_FrameDataRegistry.GetCurrentData(registryId, m_InFlightFrameIndex); };

    private:
        void CreateSwapChain();
        void CleanupSwapChain();
        void RecreateSwapChain();

        void CreateRenderPass();
        void CleanupRenderPass();

        void CreateFramebufferImages();
        void CleanupFramebufferImages();

        void CreateFramebuffers();
        void CleanupFramebuffers();

        void AllocateCommandBuffers();
        void FreeCommandBuffers();

        void CreateSynchronizationObjects();
        void CleanupSynchronizationObjects();

        void Present();

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    private:
        glm::vec4 m_ClearColor = { 0.f, 0.f, 0.f, 1.f };

        bool m_Initialized = false;
        int m_Width, m_Height;
        VkSurfaceKHR m_Surface;
        VkSwapchainKHR m_SwapChain;
        SwapChainData m_SwapChainData;
        VkRenderPass m_RenderPass;
        std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_CommandBuffers; // secondary
        std::vector<VkCommandBuffer> m_AuxiliaryCommandBuffers = {}; // primary submitted from framebuffers
        std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_ImageAvailableSemaphores;
        std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_RenderFinishedSemaphores;
        std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_AuxiliaryRenderFinishedSemaphores;
        std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_InFlightFences;
        std::vector<VkFence> m_ImagesInFlight = {};
        u32 m_PresentImageIndex;
        bool m_ShouldPresentThisFrame;
        u32 m_InFlightFrameIndex = 0;
        bool m_SwapChainInvalid = false;
        FrameDataRegistry m_FrameDataRegistry;

        VkImage m_ColorImage;
        VkImage m_DepthImage;

        VkImageView m_ColorImageView;
        VkImageView m_DepthImageView;

        VkDeviceMemory m_ColorImageMemory;
        VkDeviceMemory m_DepthImageMemory;
    };
}