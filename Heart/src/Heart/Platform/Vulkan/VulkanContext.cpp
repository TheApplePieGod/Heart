#include "hepch.h"
#include "VulkanContext.h"

#include "Heart/Core/Timing.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Platform/Vulkan/VulkanFramebuffer.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "GLFW/glfw3.h"

namespace Heart
{
    u32 VulkanContext::s_ContextCount = 0;
    VkInstance VulkanContext::s_Instance = nullptr;
    VkDebugUtilsMessengerEXT VulkanContext::s_DebugMessenger = nullptr;
    VulkanDevice VulkanContext::s_VulkanDevice;
    VkCommandPool VulkanContext::s_GraphicsPool;
    VkCommandPool VulkanContext::s_ComputePool;
    VkCommandPool VulkanContext::s_TransferPool;
    VulkanFramebuffer* VulkanContext::s_BoundFramebuffer = nullptr;
    VkSampler VulkanContext::s_DefaultSampler;
    std::deque<std::function<void()>> VulkanContext::s_DeleteQueue;
    EventEmitter VulkanContext::s_EventEmitter;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        switch (messageSeverity)
        {
            default:
                break;
            //case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: HE_ENGINE_LOG_TRACE("Vulkan Validation: {0}", pCallbackData->pMessage); break;
            //case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: HE_ENGINE_LOG_INFO("Vulkan Validation: {0}", pCallbackData->pMessage); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: HE_ENGINE_LOG_WARN("Vulkan Validation: {0}", pCallbackData->pMessage); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: HE_ENGINE_LOG_ERROR("Vulkan Validation: {0}", pCallbackData->pMessage); break;
        }

        return VK_FALSE;
    }

    VulkanContext::VulkanContext(void* window)
    {
        m_WindowHandle = window;
        if (s_ContextCount == 0)
        {
            HE_ENGINE_LOG_TRACE("VULKAN: Initializing instance");
            InitializeInstance();

            HE_ENGINE_LOG_TRACE("VULKAN: Creating window surface");
            CreateSurface(m_Surface);

            HE_ENGINE_LOG_TRACE("VULKAN: Initializing devices");
            s_VulkanDevice.Initialize(m_Surface);

            InitializeCommandPools();
            CreateDefaultSampler();
        }
        else
            CreateSurface(m_Surface);

        int width, height;
        glfwGetFramebufferSize((GLFWwindow*)window, &width, &height);
        HE_ENGINE_LOG_TRACE("VULKAN: Creating swapchain");
        m_VulkanSwapChain.Initialize(width, height, m_Surface);

        CreateImGuiDescriptorPool();

        s_ContextCount++;
    }

    VulkanContext::~VulkanContext()
    {
        HE_ENGINE_LOG_TRACE("VULKAN: Destroying context");
        Sync();

        ProcessDeleteQueue();

        m_VulkanSwapChain.Shutdown();

        vkDestroySurfaceKHR(s_Instance, m_Surface, nullptr);

        CleanupImGuiDescriptorPool();

        s_ContextCount--;
        if (s_ContextCount == 0)
        {
            HE_ENGINE_LOG_TRACE("VULKAN: No context instances left, shutting down");
            vkDestroySampler(s_VulkanDevice.Device(), s_DefaultSampler, nullptr);

            vkDestroyCommandPool(s_VulkanDevice.Device(), s_GraphicsPool, nullptr);
            vkDestroyCommandPool(s_VulkanDevice.Device(), s_ComputePool, nullptr);
            vkDestroyCommandPool(s_VulkanDevice.Device(), s_TransferPool, nullptr);

            s_VulkanDevice.Shutdown();

            #if HE_DEBUG
                // find func and destroy debug instance
                auto destroyDebugUtilsFunc = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(s_Instance, "vkDestroyDebugUtilsMessengerEXT");
                HE_ENGINE_ASSERT(destroyDebugUtilsFunc != nullptr);
                destroyDebugUtilsFunc(s_Instance, s_DebugMessenger, nullptr);
            #endif

            vkDestroyInstance(s_Instance, nullptr);
        }
    }

    void VulkanContext::CreateSurface(VkSurfaceKHR& surface)
    {
        // window pointer should always be a GLFW window (for now)
        HE_VULKAN_CHECK_RESULT(glfwCreateWindowSurface(s_Instance, (GLFWwindow*)m_WindowHandle, nullptr, &surface));
    }

    void VulkanContext::CreateImGuiDescriptorPool()
    {
        u32 maxSets = 10000;

        std::array<VkDescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = maxSets;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxSets;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        HE_VULKAN_CHECK_RESULT(vkCreateDescriptorPool(s_VulkanDevice.Device(), &poolInfo, nullptr, &m_ImGuiDescriptorPool));
    }

    void VulkanContext::CleanupImGuiDescriptorPool()
    {
        vkDestroyDescriptorPool(s_VulkanDevice.Device(), m_ImGuiDescriptorPool, nullptr);
    }

    void VulkanContext::InitializeInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        // TODO: fill these out?
        //appInfo.pApplicationName = windowName;
        //appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Heart";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        // get api extension support
        u32 supportedExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
        HVector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.Data());

        // get required extensions
        u32 glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // check for compatability (TODO: make this better)
        for (u32 i = 0; i < glfwExtensionCount; i++)
        {
            HE_ENGINE_ASSERT(std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [&glfwExtensions, &i](const VkExtensionProperties& arg) { return strcmp(arg.extensionName, glfwExtensions[i]); }) != supportedExtensions.end());
        }

        // create a new extensions array to add debug item
        HVector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // finally create instance
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        #if HE_DEBUG
            HE_ENGINE_LOG_TRACE("VULKAN: Configuring validation layers");
            auto validationLayers = ConfigureValidationLayers();
            createInfo.enabledLayerCount = static_cast<u32>(validationLayers.GetCount());
            createInfo.ppEnabledLayerNames = validationLayers.Data();
            extensions.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #else
            createInfo.enabledLayerCount = 0;
        #endif
        createInfo.enabledExtensionCount = static_cast<u32>(extensions.GetCount());
        createInfo.ppEnabledExtensionNames = extensions.Data();

        HE_VULKAN_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &s_Instance));

        #if HE_DEBUG
            // setup debug messenger
            {
                VkDebugUtilsMessengerCreateInfoEXT createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                createInfo.pfnUserCallback = debugCallback;
                createInfo.pUserData = nullptr; // Optional

                // locate extension creation function and run
                auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(s_Instance, "vkCreateDebugUtilsMessengerEXT");
                HE_ENGINE_ASSERT(func != nullptr)
                HE_VULKAN_CHECK_RESULT(func(s_Instance, &createInfo, nullptr, &s_DebugMessenger));
            }   
        #endif
    }

    void VulkanContext::InitializeCommandPools()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = s_VulkanDevice.GraphicsQueueIndex();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow resetting

        HE_VULKAN_CHECK_RESULT(vkCreateCommandPool(s_VulkanDevice.Device(), &poolInfo, nullptr, &s_GraphicsPool));
        
        poolInfo.queueFamilyIndex = s_VulkanDevice.ComputeQueueIndex();

        HE_VULKAN_CHECK_RESULT(vkCreateCommandPool(s_VulkanDevice.Device(), &poolInfo, nullptr, &s_ComputePool));

        poolInfo.queueFamilyIndex = s_VulkanDevice.TransferQueueIndex();

        HE_VULKAN_CHECK_RESULT(vkCreateCommandPool(s_VulkanDevice.Device(), &poolInfo, nullptr, &s_TransferPool));
    }

    void VulkanContext::CreateDefaultSampler()
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(s_VulkanDevice.PhysicalDevice(), &properties);
        
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        HE_VULKAN_CHECK_RESULT(vkCreateSampler(s_VulkanDevice.Device(), &samplerInfo, nullptr, &s_DefaultSampler));
    }

    void VulkanContext::InitializeImGui()
    {
        // TODO: variable samples
        ImGui_ImplVulkan_InitInfo info = {};
        info.Instance = s_Instance;
        info.PhysicalDevice = s_VulkanDevice.PhysicalDevice();
        info.Device = s_VulkanDevice.Device();
        info.QueueFamily = s_VulkanDevice.GraphicsQueueIndex();
        info.Queue = s_VulkanDevice.PresentQueue();
        info.PipelineCache = VK_NULL_HANDLE;
        info.DescriptorPool = m_ImGuiDescriptorPool;
        info.Allocator = NULL;
        info.MinImageCount = 2;
        info.ImageCount = m_VulkanSwapChain.GetImageCount();
        info.CheckVkResultFn = NULL;
        info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; //s_VulkanDevice.MaxMsaaSamples();

        ImGui_ImplVulkan_Init(&info, m_VulkanSwapChain.GetRenderPass());
        VkCommandBuffer commandBuffer = VulkanCommon::BeginSingleTimeCommands(s_VulkanDevice.Device(), s_GraphicsPool);
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        VulkanCommon::EndSingleTimeCommands(s_VulkanDevice.Device(), s_GraphicsPool, commandBuffer, s_VulkanDevice.GraphicsQueue());
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void VulkanContext::ShutdownImGui()
    {
        Sync();

        ProcessDeleteQueue();

        ImGui_ImplVulkan_Shutdown();
    }

    void VulkanContext::ImGuiBeginFrame()
    {
        HE_PROFILE_FUNCTION();

        ImGui_ImplVulkan_NewFrame();
    }

    void VulkanContext::ImGuiEndFrame()
    {
        HE_PROFILE_FUNCTION();

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_VulkanSwapChain.GetCommandBuffer());
    }

    void VulkanContext::BeginFrame()
    {
        HE_PROFILE_FUNCTION();

        ProcessDeleteQueue();

        m_VulkanSwapChain.BeginFrame();
    }

    void VulkanContext::EndFrame()
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("VulkanContext::EndFrame");
        
        m_VulkanSwapChain.EndFrame();
        s_BoundFramebuffer = nullptr;
    }

    void VulkanContext::ProcessDeleteQueue()
    {
        if (!s_DeleteQueue.empty())
        {
            Sync();

            // Reverse iterate (FIFO)
		    for (auto it = s_DeleteQueue.rbegin(); it != s_DeleteQueue.rend(); it++)
			    (*it)(); // Call the function

            s_DeleteQueue.clear();
		}
    }

    HVector<const char*> VulkanContext::ConfigureValidationLayers()
    {
        std::array<const char*, 1> requestedLayers = { "VK_LAYER_KHRONOS_validation" };
        HVector<const char*> finalLayers;

        u32 supportedLayerCount;
        vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);
        HVector<VkLayerProperties> supportedLayers(supportedLayerCount);
        vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.Data());

        // check for compatability
        for (auto rl : requestedLayers)
        {
            for (auto sl : supportedLayers)
            {
                if (strcmp(rl, sl.layerName) == 0)
                {
                    finalLayers.AddInPlace(rl);
                    break;
                }
            }
        }

        return finalLayers;
    }

}