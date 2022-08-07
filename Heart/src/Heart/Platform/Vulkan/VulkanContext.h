#pragma once

#include "Heart/Events/EventEmitter.h"
#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "Heart/Platform/Vulkan/VulkanSwapChain.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanFramebuffer;
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
        inline static void EmitEvent(Event& event) { s_EventEmitter.Emit(event); }
        inline static EventEmitter& GetEventEmitter() { return s_EventEmitter; }

        static void ProcessJobQueue();

        // both bound command & frame buffers should be from the same object
        inline static VulkanFramebuffer* GetBoundFramebuffer() { HE_ENGINE_ASSERT(s_BoundFramebuffer != nullptr, "No framebuffer is bound (forget to call Framebuffer.Bind()?)"); return s_BoundFramebuffer; }
        inline static void SetBoundFramebuffer(VulkanFramebuffer* buffer) { s_BoundFramebuffer = buffer; }

        static HVector<const char*> ConfigureValidationLayers();
    
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
        inline static u32 s_ContextCount = 0;
        inline static VkInstance s_Instance = nullptr;
        inline static VkDebugUtilsMessengerEXT s_DebugMessenger = nullptr;
        inline static VulkanDevice s_VulkanDevice;
        inline static VkCommandPool s_GraphicsPool, s_ComputePool, s_TransferPool;
        inline static VulkanFramebuffer* s_BoundFramebuffer = nullptr;
        inline static VkSampler s_DefaultSampler;
        inline static EventEmitter s_EventEmitter;
    };
}