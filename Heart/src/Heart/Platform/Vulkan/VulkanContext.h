#pragma once

#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "Heart/Platform/Vulkan/VulkanSwapChain.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanContext : public GraphicsContext
    {
    public:
        VulkanContext(void* window);
        ~VulkanContext();

    public:
        inline static VkInstance GetInstance() { return s_Instance; };
        inline static VulkanDevice& GetDevice() { return s_VulkanDevice; };

        static std::vector<const char*> ConfigureValidationLayers();
    
    private:
        void CreateSurface(VkSurfaceKHR& surface);

    private:
        void* m_WindowHandle;
        VkSurfaceKHR m_Surface;
        VulkanSwapChain m_VulkanSwapChain;

    private:
        static void InitializeInstance();

    private:
        static u32 s_ContextCount;
        static VkInstance s_Instance;
        static VkDebugUtilsMessengerEXT s_DebugMessenger;
        static VulkanDevice s_VulkanDevice;
    };
}