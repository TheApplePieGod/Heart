#pragma once

#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Container/HVector.hpp"

namespace Heart
{
    struct SwapChainData
    {
        VkFormat ImageFormat;
        VkExtent2D Extent;
        HVector<VkImage> Images;
        HVector<VkImageView> ImageViews;
        HVector<VkFramebuffer> Framebuffers;
        HVector<VkCommandBuffer> CommandBuffers;
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
        void SubmitCommandBuffers(const HVector<VulkanFramebufferSubmit>& submits);

        void InvalidateSwapChain(u32 newWidth, u32 newHeight);

        inline u32 GetImageCount() const { return static_cast<u32>(m_SwapChainData.Images.GetCount()); }
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

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const HVector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR ChooseSwapPresentMode(const HVector<VkPresentModeKHR>& presentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    private:
        glm::vec4 m_ClearColor = { 0.f, 0.f, 0.f, 1.f };

        bool m_Initialized = false;
        int m_Width, m_Height;
        VkSurfaceKHR m_Surface;
        VkSwapchainKHR m_SwapChain;
        SwapChainData m_SwapChainData;
        VkRenderPass m_RenderPass;
        std::array<VkCommandBuffer, Renderer::FrameBufferCount> m_CommandBuffers; // secondary

        HVector<VkCommandBuffer> m_SubmittedCommandBuffers{};
        HVector<FramebufferSubmissionData> m_FramebufferSubmissions = {}; // collection of all submitted framebuffers

        std::array<VkSemaphore, Renderer::FrameBufferCount> m_ImageAvailableSemaphores;
        std::array<VkSemaphore, Renderer::FrameBufferCount> m_RenderFinishedSemaphores;
        HVector<VkSemaphore> m_AuxiliaryRenderFinishedSemaphores = {};
        HVector<VkSemaphore> m_AuxiliaryComputeFinishedSemaphores = {};
        std::array<VkFence, Renderer::FrameBufferCount> m_InFlightFences;
        std::array<VkFence, Renderer::FrameBufferCount> m_InFlightTransferFences;
        std::array<VkFence, Renderer::FrameBufferCount> m_InFlightComputeFences;
        HVector<VkFence> m_ImagesInFlight = {};
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