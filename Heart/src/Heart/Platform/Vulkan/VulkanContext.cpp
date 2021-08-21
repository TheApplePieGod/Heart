#include "htpch.h"
#include "VulkanContext.h"

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
    VkCommandBuffer VulkanContext::s_BoundCommandBuffer;
    VkSampler VulkanContext::s_DefaultSampler;

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
        HE_ENGINE_LOG_TRACE("Initializing new vulkan context");

        m_WindowHandle = window;
        if (s_ContextCount == 0)
        {
            HE_ENGINE_LOG_INFO("Initializing vulkan instance");
            InitializeInstance();

            CreateSurface(m_Surface);

            HE_ENGINE_LOG_INFO("Initializing vulkan devices");
            s_VulkanDevice.Initialize(m_Surface);

            InitializeCommandPools();

            CreateDefaultSampler();
        }
        else
            CreateSurface(m_Surface);

        int width, height;
        glfwGetFramebufferSize((GLFWwindow*)window, &width, &height);
        m_VulkanSwapChain.Initialize(width, height, m_Surface);

        CreateImGuiDescriptorPool();

        s_ContextCount++;
    }

    VulkanContext::~VulkanContext()
    {
        HE_ENGINE_LOG_TRACE("Destructing vulkan context");
        vkDeviceWaitIdle(s_VulkanDevice.Device());

        m_VulkanSwapChain.Shutdown();

        vkDestroySurfaceKHR(s_Instance, m_Surface, nullptr);

        CleanupImGuiDescriptorPool();

        s_ContextCount--;
        if (s_ContextCount == 0)
        {
            HE_ENGINE_LOG_INFO("Cleaning up vulkan");
            vkDestroySampler(s_VulkanDevice.Device(), s_DefaultSampler, nullptr);

            vkDestroyCommandPool(s_VulkanDevice.Device(), s_GraphicsPool, nullptr);
            vkDestroyCommandPool(s_VulkanDevice.Device(), s_ComputePool, nullptr);

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
        std::array<VkDescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = static_cast<u32>(m_VulkanSwapChain.GetImageCount());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 100;
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
        appInfo.apiVersion = VK_API_VERSION_1_1;

        // get api extension support
        u32 supportedExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
        std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());

        // get required extensions
        u32 glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // check for compatability
        for (u32 i = 0; i < glfwExtensionCount; i++)
        {
            HE_ENGINE_ASSERT(std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [&glfwExtensions, &i](const VkExtensionProperties& arg) { return strcmp(arg.extensionName, glfwExtensions[i]); }) != supportedExtensions.end());
        }

        // create a new extensions array to add debug item
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // finally create instance
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        #if HE_DEBUG
            auto validationLayers = ConfigureValidationLayers();
            createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #else
            createInfo.enabledLayerCount = 0;
        #endif
        createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

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
        info.MSAASamples = s_VulkanDevice.MaxMsaaSamples();

        ImGui_ImplVulkan_Init(&info, m_VulkanSwapChain.GetRenderPass());
        VkCommandBuffer commandBuffer = VulkanCommon::BeginSingleTimeCommands(s_VulkanDevice.Device(), s_GraphicsPool);
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        VulkanCommon::EndSingleTimeCommands(s_VulkanDevice.Device(), s_GraphicsPool, commandBuffer, s_VulkanDevice.GraphicsQueue());
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void VulkanContext::ShutdownImGui()
    {
        vkDeviceWaitIdle(s_VulkanDevice.Device());
        ImGui_ImplVulkan_Shutdown();
    }

    void VulkanContext::ImGuiBeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
    }

    void VulkanContext::ImGuiEndFrame()
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_VulkanSwapChain.GetCommandBuffer());
    }

    void VulkanContext::BeginFrame()
    {
        m_VulkanSwapChain.BeginFrame();

        // bind the initial commandbuffer to be the main window's
        SetBoundCommandBuffer(m_VulkanSwapChain.GetCommandBuffer());
    }

    void VulkanContext::EndFrame()
    {
        m_VulkanSwapChain.EndFrame();
    }

    std::vector<const char*> VulkanContext::ConfigureValidationLayers()
    {
        std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

        u32 supportedLayerCount;
        vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);
        std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
        vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

        // check for compatability
        for (int i = 0; i < validationLayers.size(); i++)
        {
            HE_ENGINE_ASSERT(std::find_if(supportedLayers.begin(), supportedLayers.end(), [&validationLayers, &i](const VkLayerProperties& arg) { return strcmp(arg.layerName, validationLayers[i]); }) != supportedLayers.end());
        }

        return validationLayers;
    }

}