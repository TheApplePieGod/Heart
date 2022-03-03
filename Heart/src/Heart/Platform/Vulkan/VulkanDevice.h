#pragma once

#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanDevice
    {
    public:
        void Initialize(VkSurfaceKHR mainWindowSurface);
        void Shutdown();

    public:
        inline VkPhysicalDevice PhysicalDevice() const { return m_PhysicalDevice; }
        inline VkDevice Device() const { return m_LogicalDevice; }
        inline VkQueue PresentQueue() const { return m_PresentQueues[0]; }
        inline VkQueue GraphicsQueue() const { return m_GraphicsQueues[0]; }
        inline VkQueue ComputeQueue() const { return m_ComputeQueues[0]; }
        inline VkQueue TransferQueue() const { return m_TransferQueues[0]; }
        inline VkQueue PresentQueue(u32 inFlightFrameIndex) const { return m_PresentQueues[inFlightFrameIndex]; }
        inline VkQueue GraphicsQueue(u32 inFlightFrameIndex) const { return m_GraphicsQueues[inFlightFrameIndex]; }
        inline VkQueue ComputeQueue(u32 inFlightFrameIndex) const { return m_ComputeQueues[inFlightFrameIndex]; }
        inline VkQueue TransferQueue(u32 inFlightFrameIndex) const { return m_TransferQueues[inFlightFrameIndex]; }
        inline u32 GraphicsQueueIndex() const { return m_GraphicsQueueIndex; }
        inline u32 PresentQueueIndex() const { return m_PresentQueueIndex; }
        inline u32 ComputeQueueIndex() const { return m_ComputeQueueIndex; }
        inline u32 TransferQueueIndex() const { return m_TransferQueueIndex; }
        inline VkSampleCountFlagBits MaxMsaaSamples() const { return m_DeviceMaxSampleCount; }
        inline bool IsInitialized() const { return m_Initialized; }
        inline const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const { return m_PhysicalDeviceProperties; }

    private:
        struct OptionalDeviceFeatures
        {
            bool SamplerAnisotropy;
            bool WideLines;
        };

    private:
        bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions, OptionalDeviceFeatures& outOptionalFeatures);
        VkSampleCountFlagBits GetMaxSampleCount();

    private:
        VkPhysicalDevice m_PhysicalDevice;
        VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
        VkSampleCountFlagBits m_DeviceMaxSampleCount;
        VkDevice m_LogicalDevice;
        std::array<VkQueue, MAX_FRAMES_IN_FLIGHT> m_GraphicsQueues, m_ComputeQueues, m_TransferQueues, m_PresentQueues;
        u32 m_GraphicsQueueIndex, m_PresentQueueIndex, m_ComputeQueueIndex, m_TransferQueueIndex;
        bool m_Initialized = false;
    };
}