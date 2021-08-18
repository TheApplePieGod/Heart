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
        inline VkQueue GraphicsQueue() const { return m_GraphicsQueue; }
        inline VkQueue PresentQueue() const { return m_PresentQueue; }
        inline VkQueue ComputeQueue() const { return m_ComputeQueue; }
        inline u32 GraphicsQueueIndex() const { return m_GraphicsQueueIndex; }
        inline u32 PresentQueueIndex() const { return m_PresentQueueIndex; }
        inline u32 ComputeQueueIndex() const { return m_ComputeQueueIndex; }
        inline VkSampleCountFlagBits MaxMsaaSamples() const { return m_DeviceMaxSampleCount; }
        inline bool IsInitialized() const { return m_Initialized; }

    private:
        bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions);
        VkSampleCountFlagBits GetMaxSampleCount();

    private:
        VkPhysicalDevice m_PhysicalDevice;
        VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
        VkSampleCountFlagBits m_DeviceMaxSampleCount;
        VkDevice m_LogicalDevice;
        VkQueue m_GraphicsQueue, m_PresentQueue, m_ComputeQueue;
        u32 m_GraphicsQueueIndex, m_PresentQueueIndex, m_ComputeQueueIndex;
        bool m_Initialized = false;
    };
}