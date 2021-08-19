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
        ~VulkanContext() override;

        void InitializeImGui() override;
        void ShutdownImGui() override;
        void ImGuiBeginFrame() override;
        void ImGuiEndFrame() override;

        void BeginFrame() override;
        void EndFrame() override;

        inline VkRenderPass GetMainRenderPass() const { return m_VulkanSwapChain.GetRenderPass(); }
        inline VkCommandBuffer GetMainCommandBuffer() const { return m_VulkanSwapChain.GetCommandBuffer(); }
        inline VulkanSwapChain& GetSwapChain() { return m_VulkanSwapChain; };

    public:
        inline static VkInstance GetInstance() { return s_Instance; };
        inline static VulkanDevice& GetDevice() { return s_VulkanDevice; };
        inline static VkCommandPool GetGraphicsPool() { return s_GraphicsPool; }
        inline static VkCommandPool GetComputePool() { return s_ComputePool; }

        static std::vector<const char*> ConfigureValidationLayers();
    
    private:
        void CreateSurface(VkSurfaceKHR& surface);

        void CreateImGuiDescriptorPool();
        void CleanupImGuiDescriptorPool();

    private:
        void* m_WindowHandle;
        VkSurfaceKHR m_Surface;
        VulkanSwapChain m_VulkanSwapChain;
        VkDescriptorPool m_ImGuiDescriptorPool;

    private:
        static void InitializeInstance();
        static void InitializeCommandPools();

    private:
        static u32 s_ContextCount;
        static VkInstance s_Instance;
        static VkDebugUtilsMessengerEXT s_DebugMessenger;
        static VulkanDevice s_VulkanDevice;
        static VkCommandPool s_GraphicsPool;
        static VkCommandPool s_ComputePool;
    };
}