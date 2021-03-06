#pragma once

#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    struct SwapChainData
    {
        VkFormat ImageFormat;
        VkExtent2D Extent;
        std::vector<VkImage> Images;
        std::vector<VkImageView> ImageViews;
        std::vector<VkFramebuffer> Framebuffers;
        std::vector<VkCommandBuffer> CommandBuffers;
    };
    struct VulkanFramebufferSubmit
    {
        // The buffer holding graphics commands
        VkCommandBuffer DrawBuffer = nullptr;

        // The buffer holding transfer commands that rely on the completion of the draw buffer
        VkCommandBuffer TransferBuffer = nullptr;
    };
    struct VulkanComputePipelineSubmit
    {
        VkCommandBuffer CommandBuffer = nullptr;
    };

    class VulkanSwapChain
    {
    public:
        void Initialize(int width, int height, VkSurfaceKHR surface);
        void Shutdown();

        void BeginFrame();
        void EndFrame();
        void SubmitCommandBuffers(const std::vector<VulkanFramebufferSubmit>& submits);

        void InvalidateSwapChain(u32 newWidth, u32 newHeight);

        inline u32 GetImageCount() const { return static_cast<u32>(m_SwapChainData.Images.size()); }
        inline VkFormat GetImageFormat() const { return m_SwapChainData.ImageFormat; }
        inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
        inline VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffers[m_InFlightFrameIndex]; }
        inline u32 GetPresentImageIndex() const { return m_PresentImageIndex; }
        inline u32 GetInFlightFrameIndex() const { return m_InFlightFrameIndex; }

    private:
        struct FramebufferSubmissionData
        {
            u32 DrawBufferStartIndex;
            u32 DrawBufferCount;

            u32 TransferBufferStartIndex;
            u32 TransferBufferCount;
        };
        struct ComputeSubmissionData
        {
            u32 CommandBufferStartIndex;
            u32 CommandBufferCount;
        };

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

        // used when submitting framebuffers
        VkSemaphore GetAuxiliaryRenderSemaphore(size_t renderIndex);
        VkSemaphore GetAuxiliaryComputeSemaphore(size_t renderIndex);

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

        std::vector<VkCommandBuffer> m_SubmittedCommandBuffers{};
        std::vector<FramebufferSubmissionData> m_FramebufferSubmissions = {}; // collection of all submitted framebuffers

        std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_ImageAvailableSemaphores;
        std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_RenderFinishedSemaphores;
        std::vector<VkSemaphore> m_AuxiliaryRenderFinishedSemaphores = {};
        std::vector<VkSemaphore> m_AuxiliaryComputeFinishedSemaphores = {};
        std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_InFlightFences;
        std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_InFlightTransferFences;
        std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_InFlightComputeFences;
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