#include "hepch.h"
#include "VulkanDevice.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "GLFW/glfw3.h"

namespace Heart
{
    void VulkanDevice::Initialize(VkSurfaceKHR mainWindowSurface)
    {
        if (m_Initialized) return;

        VkInstance instance = VulkanContext::GetInstance();

        // setup physical device
        std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME /*, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME*/ };
        OptionalDeviceFeatures optionalFeatures;
        auto validationLayers = VulkanContext::ConfigureValidationLayers();
        {
            // get devices
            u32 deviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
            HE_ENGINE_ASSERT(deviceCount != 0);
            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

            // find first compatible device
            for (const auto& device: devices)
            {
                if (IsDeviceSuitable(device, mainWindowSurface, deviceExtensions, optionalFeatures))
                {
                    m_PhysicalDevice = device;
                    vkGetPhysicalDeviceProperties(device, &m_PhysicalDeviceProperties);
                    m_DeviceMaxSampleCount = GetMaxSampleCount();
                    break;
                }
            }
            HE_ENGINE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Could not find a suitable GPU");
        }

        // setup logical device & queues
        {
            VulkanCommon::QueueFamilyIndices indices = VulkanCommon::GetQueueFamilies(m_PhysicalDevice, mainWindowSurface);
            float queuePriority = 1.0f;

            // info for creating queues
            std::unordered_set<u32> uniqueFamilies = { indices.GraphicsFamily.value(), indices.PresentFamily.value(), indices.ComputeFamily.value() };
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            for (u32 family : uniqueFamilies)
            {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = family;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            VkPhysicalDeviceFeatures deviceFeatures{};

            // required device features
            deviceFeatures.independentBlend = VK_TRUE;
            deviceFeatures.multiDrawIndirect = VK_TRUE;
            //deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;

            // optional device features
            deviceFeatures.samplerAnisotropy = optionalFeatures.SamplerAnisotropy;
            deviceFeatures.wideLines = optionalFeatures.WideLines;
            
            VkPhysicalDeviceRobustness2FeaturesEXT robustnessFeatures{};
            robustnessFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
            robustnessFeatures.pNext = nullptr;

            // VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{};
            // indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            // indexingFeatures.pNext = nullptr;
            // indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
            // indexingFeatures.runtimeDescriptorArray = VK_TRUE;

            // finally create device
            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            //createInfo.pNext = &indexingFeatures;
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());;
            createInfo.pEnabledFeatures = &deviceFeatures;
            createInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());;
            createInfo.ppEnabledExtensionNames = deviceExtensions.data();
            #if HE_DEBUG
                createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();
            #else
                createInfo.enabledLayerCount = 0;
            #endif

            HE_VULKAN_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice));

            m_GraphicsQueueIndex = indices.GraphicsFamily.value();
            m_PresentQueueIndex = indices.PresentFamily.value();
            m_ComputeQueueIndex = indices.ComputeFamily.value();
            m_TransferQueueIndex = indices.TransferFamily.value();
            vkGetDeviceQueue(m_LogicalDevice, m_GraphicsQueueIndex, 0, &m_GraphicsQueue);
            vkGetDeviceQueue(m_LogicalDevice, m_PresentQueueIndex, 0, &m_PresentQueue);
            vkGetDeviceQueue(m_LogicalDevice, m_ComputeQueueIndex, 0, &m_ComputeQueue);
            vkGetDeviceQueue(m_LogicalDevice, m_TransferQueueIndex, 0, &m_TransferQueue);
        }

        m_Initialized = true;
    }

    bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions, OptionalDeviceFeatures& outOptionalFeatures)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        VulkanCommon::QueueFamilyIndices indices = VulkanCommon::GetQueueFamilies(device, surface);

        VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{};
        indexingFeatures.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
        indexingFeatures.pNext = nullptr;
        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &indexingFeatures;
        vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);

        // get hardware extension support
        u32 supportedExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &supportedExtensionCount, nullptr);
        std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &supportedExtensionCount, supportedExtensions.data());

        // check for compatability
        int count = 0;
        for (int i = 0; i < deviceExtensions.size(); i++)
        {
            // device is not supported if any extension is not supported, so return false
            if (std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [&deviceExtensions, &i](const VkExtensionProperties& arg) { return strcmp(arg.extensionName, deviceExtensions[i]); }) == supportedExtensions.end())
                return false;
        }

        VulkanCommon::SwapChainSupportDetails swapDetails = VulkanCommon::GetSwapChainSupport(device, surface);
        bool swapChainAdequate = !swapDetails.Formats.empty() && !swapDetails.PresentModes.empty();

        outOptionalFeatures.SamplerAnisotropy = deviceFeatures.samplerAnisotropy;
        outOptionalFeatures.WideLines = deviceFeatures.wideLines;

        return (
            indices.IsComplete() &&
            swapChainAdequate &&
            deviceFeatures.independentBlend &&
            //deviceFeatures.shaderSampledImageArrayDynamicIndexing &&
            deviceFeatures.multiDrawIndirect
            //indexingFeatures.descriptorBindingPartiallyBound &&
            //indexingFeatures.runtimeDescriptorArray
        );
    }

    VkSampleCountFlagBits VulkanDevice::GetMaxSampleCount()
    {
        VkSampleCountFlags counts = m_PhysicalDeviceProperties.limits.framebufferColorSampleCounts & m_PhysicalDeviceProperties.limits.framebufferDepthSampleCounts;

        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    void VulkanDevice::Shutdown()
    {
        if (!m_Initialized) return;

        vkDestroyDevice(m_LogicalDevice, nullptr);

        m_Initialized = false;
    }
}