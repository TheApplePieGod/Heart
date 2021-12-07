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
        inline VkQueue TransferQueue() const { return m_TransferQueue; }
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
        VkQueue m_GraphicsQueue, m_PresentQueue, m_ComputeQueue, m_TransferQueue;
        u32 m_GraphicsQueueIndex, m_PresentQueueIndex, m_ComputeQueueIndex, m_TransferQueueIndex;
        bool m_Initialized = false;
    };
}