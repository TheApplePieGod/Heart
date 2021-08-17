#include "htpch.h"
#include "VulkanContext.h"

#include "GLFW/glfw3.h"

namespace Heart
{
    u32 VulkanContext::s_ContextCount = 0;
    VkInstance VulkanContext::s_Instance = nullptr;
    VkDebugUtilsMessengerEXT VulkanContext::s_DebugMessenger = nullptr;
    VulkanDevice VulkanContext::s_VulkanDevice;

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
            //case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: HT_ENGINE_LOG_TRACE("Vulkan Validation: {0}", pCallbackData->pMessage); break;
            //case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: HT_ENGINE_LOG_INFO("Vulkan Validation: {0}", pCallbackData->pMessage); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: HT_ENGINE_LOG_WARN("Vulkan Validation: {0}", pCallbackData->pMessage); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: HT_ENGINE_LOG_ERROR("Vulkan Validation: {0}", pCallbackData->pMessage); break;
        }

        return VK_FALSE;
    }

    VulkanContext::VulkanContext(void* window)
    {
        HT_ENGINE_LOG_TRACE("Initializing new vulkan context");

        m_WindowHandle = window;
        if (s_ContextCount == 0)
        {
            HT_ENGINE_LOG_INFO("Initializing vulkan instance");
            InitializeInstance();
            
            CreateSurface(m_Surface);

            HT_ENGINE_LOG_INFO("Initializing vulkan devices");
            s_VulkanDevice.Initialize(m_Surface);
        }
        else
            CreateSurface(m_Surface);

        int width, height;
        glfwGetFramebufferSize((GLFWwindow*)window, &width, &height);
        m_VulkanSwapChain.Initialize(width, height, m_Surface);

        s_ContextCount++;
    }

    VulkanContext::~VulkanContext()
    {
        HT_ENGINE_LOG_TRACE("Destructing vulkan context");

        m_VulkanSwapChain.Shutdown();

        vkDestroySurfaceKHR(s_Instance, m_Surface, nullptr);

        s_ContextCount--;
        if (s_ContextCount == 0)
        {
            HT_ENGINE_LOG_INFO("Cleaning up vulkan");
            s_VulkanDevice.Shutdown();

            #if HT_DEBUG
                // find func and destroy debug instance
                auto destroyDebugUtilsFunc = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(s_Instance, "vkDestroyDebugUtilsMessengerEXT");
                HT_ENGINE_ASSERT(destroyDebugUtilsFunc != nullptr);
                destroyDebugUtilsFunc(s_Instance, s_DebugMessenger, nullptr);
            #endif

            vkDestroyInstance(s_Instance, nullptr);
        }
    }

    void VulkanContext::CreateSurface(VkSurfaceKHR& surface)
    {
        // window pointer should always be a GLFW window (for now)
        HT_VULKAN_CHECK_RESULT(glfwCreateWindowSurface(s_Instance, (GLFWwindow*)m_WindowHandle, nullptr, &surface));
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
            HT_ENGINE_ASSERT(std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [&glfwExtensions, &i](const VkExtensionProperties& arg) { return strcmp(arg.extensionName, glfwExtensions[i]); }) != supportedExtensions.end());
        }

        // create a new extensions array to add debug item
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // finally create instance
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        #if HT_DEBUG
            auto validationLayers = ConfigureValidationLayers();
            createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #else
            createInfo.enabledLayerCount = 0;
        #endif
        createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        HT_VULKAN_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &s_Instance));

        #if HT_DEBUG
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
                HT_ENGINE_ASSERT(func != nullptr)
                HT_VULKAN_CHECK_RESULT(func(s_Instance, &createInfo, nullptr, &s_DebugMessenger));
            }   
        #endif
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
            HT_ENGINE_ASSERT(std::find_if(supportedLayers.begin(), supportedLayers.end(), [&validationLayers, &i](const VkLayerProperties& arg) { return strcmp(arg.layerName, validationLayers[i]); }) != supportedLayers.end());
        }

        return validationLayers;
    }

}