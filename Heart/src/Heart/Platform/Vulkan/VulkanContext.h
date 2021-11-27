#pragma once

#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "Heart/Platform/Vulkan/VulkanSwapChain.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Platform/Vulkan/VulkanFramebuffer.h"

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
        inline VulkanSwapChain& GetSwapChain() { return m_VulkanSwapChain; }

    public:
        inline static VkInstance GetInstance() { return s_Instance; };
        inline static VulkanDevice& GetDevice() { return s_VulkanDevice; };
        inline static VkCommandPool GetGraphicsPool() { return s_GraphicsPool; }
        inline static VkCommandPool GetComputePool() { return s_ComputePool; }
        inline static VkCommandPool GetTransferPool() { return s_TransferPool; }
        inline static VkSampler GetDefaultSampler() { return s_DefaultSampler; }
        inline static void Sync() { vkDeviceWaitIdle(s_VulkanDevice.Device()); }
        
        static void ProcessDeleteQueue();

        // both bound command & frame buffers should be from the same object
        inline static VulkanFramebuffer* GetBoundFramebuffer() { HE_ENGINE_ASSERT(s_BoundFramebuffer != nullptr, "No framebuffer is bound (forget to call Framebuffer.Bind()?)"); return s_BoundFramebuffer; }
        inline static void SetBoundFramebuffer(VulkanFramebuffer* buffer) { s_BoundFramebuffer = buffer; }
        inline static void PushDeleteQueue(std::function<void()>&& func) { s_DeleteQueue.emplace_back(func); }

        static std::vector<const char*> ConfigureValidationLayers();
    
    private:
        void CreateSurface(VkSurfaceKHR& surface);

        void CreateImGuiDescriptorPool();
        void CleanupImGuiDescriptorPool();

    private:
        VkSurfaceKHR m_Surface;
        VulkanSwapChain m_VulkanSwapChain;
        VkDescriptorPool m_ImGuiDescriptorPool;

    private:
        static void InitializeInstance();
        static void InitializeCommandPools();
        static void CreateDefaultSampler();

    private:
        static u32 s_ContextCount;
        static VkInstance s_Instance;
        static VkDebugUtilsMessengerEXT s_DebugMessenger;
        static VulkanDevice s_VulkanDevice;
        static VkCommandPool s_GraphicsPool, s_ComputePool, s_TransferPool;
        static VulkanFramebuffer* s_BoundFramebuffer;
        static VkSampler s_DefaultSampler;
        static std::deque<std::function<void()>> s_DeleteQueue;
    };
}