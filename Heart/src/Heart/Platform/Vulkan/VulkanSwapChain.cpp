#include "hepch.h"
#include "VulkanSwapChain.h"

#include "Heart/Core/Timing.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "GLFW/glfw3.h"

namespace Heart
{
    void VulkanSwapChain::Initialize(int width, int height, VkSurfaceKHR surface)
    {
        if (m_Initialized) return;
        m_Surface = surface;
        m_Width = width;
        m_Height = height;

        VulkanDevice& device = VulkanContext::GetDevice();

        CreateSwapChain();

        CreateRenderPass();

        CreateFramebufferImages();

        CreateFramebuffers();

        AllocateCommandBuffers();

        CreateSynchronizationObjects();

        m_Initialized = true;
    }

    void VulkanSwapChain::Shutdown()
    {
        if (!m_Initialized) return;

        CleanupSynchronizationObjects();

        FreeCommandBuffers();

        CleanupFramebuffers();

        CleanupFramebufferImages();

        CleanupRenderPass();
        
        CleanupSwapChain();

        m_Initialized = false;
    }

    void VulkanSwapChain::CreateSwapChain()
    {
        VkInstance instance = VulkanContext::GetInstance();
        VulkanDevice& device = VulkanContext::GetDevice();

        VulkanCommon::SwapChainSupportDetails swapChainSupport = VulkanCommon::GetSwapChainSupport(device.PhysicalDevice(), m_Surface);
        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

        u32 imageCount = 3; //swapChainSupport.Capabilities.minImageCount + 1;
        if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
            imageCount = swapChainSupport.Capabilities.maxImageCount;

        u32 queueIndices[] = { device.GraphicsQueueIndex(), device.PresentQueueIndex() };

        m_SwapChainData.ImageFormat = surfaceFormat.format;
        m_SwapChainData.Extent = extent;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_Surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (device.GraphicsQueueIndex() != device.PresentQueueIndex())
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        HE_VULKAN_CHECK_RESULT(vkCreateSwapchainKHR(device.Device(), &createInfo, nullptr, &m_SwapChain));

        vkGetSwapchainImagesKHR(device.Device(), m_SwapChain, &imageCount, nullptr);
        m_SwapChainData.Images.resize(imageCount);
        vkGetSwapchainImagesKHR(device.Device(), m_SwapChain, &imageCount, m_SwapChainData.Images.data());

        m_SwapChainData.ImageViews.resize(m_SwapChainData.Images.size());
        for (int i = 0; i < m_SwapChainData.Images.size(); i++)
        {
            m_SwapChainData.ImageViews[i] = VulkanCommon::CreateImageView(device.Device(), m_SwapChainData.Images[i], m_SwapChainData.ImageFormat, 1, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void VulkanSwapChain::CleanupSwapChain()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        for (int i = 0; i < m_SwapChainData.ImageViews.size(); i++)
        {
            vkDestroyImageView(device.Device(), m_SwapChainData.ImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(device.Device(), m_SwapChain, nullptr);
    }

    void VulkanSwapChain::CreateRenderPass()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        // similar to VulkanFramebuffer, create the main renderpass designed to render to the swapchain
        // VkAttachmentDescription depthAttachment{};
        // depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        // depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //device.MaxMsaaSamples();
        // depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // VkAttachmentDescription colorAttachment{};
        // colorAttachment.format = m_SwapChainData.ImageFormat;
        // colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //device.MaxMsaaSamples();
        // colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        // colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = m_SwapChainData.ImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // VkAttachmentReference colorAttachmentRef{};
        // colorAttachmentRef.attachment = 0;
        // colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 0; // 1;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // VkAttachmentReference depthAttachmentRef{};
        // depthAttachmentRef.attachment = 2;
        // depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentResolveRef; // &colorAttachmentRef;
        //subpass.pResolveAttachments = &colorAttachmentResolveRef;
        //subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        //std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, colorAttachmentResolve, depthAttachment };
        std::array<VkAttachmentDescription, 1> attachments = { colorAttachmentResolve };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        HE_VULKAN_CHECK_RESULT(vkCreateRenderPass(device.Device(), &renderPassInfo, nullptr, &m_RenderPass));
    }

    void VulkanSwapChain::CleanupRenderPass()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkDestroyRenderPass(device.Device(), m_RenderPass, nullptr);
    }

    void VulkanSwapChain::CreateFramebufferImages()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkFormat colorFormat = m_SwapChainData.ImageFormat;
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

        // TODO: variable samplecount
        // VulkanCommon::CreateImage(
        //     device.Device(),
        //     device.PhysicalDevice(),
        //     m_SwapChainData.Extent.width,
        //     m_SwapChainData.Extent.height,
        //     colorFormat,
        //     1,
        //     VK_SAMPLE_COUNT_1_BIT, //device.MaxMsaaSamples(), 
        //     VK_IMAGE_TILING_OPTIMAL,
        //     VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        //     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        //     m_ColorImage,
        //     m_ColorImageMemory
        // );
        // m_ColorImageView = VulkanCommon::CreateImageView(device.Device(), m_ColorImage, colorFormat, 1);

        // VulkanCommon::CreateImage(
        //     device.Device(),
        //     device.PhysicalDevice(),
        //     m_SwapChainData.Extent.width,
        //     m_SwapChainData.Extent.height,
        //     depthFormat,
        //     1,
        //     VK_SAMPLE_COUNT_1_BIT, //device.MaxMsaaSamples(),
        //     VK_IMAGE_TILING_OPTIMAL,
        //     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        //     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        //     m_DepthImage,
        //     m_DepthImageMemory
        // );
        //m_DepthImageView = VulkanCommon::CreateImageView(device.Device(), m_DepthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);      
    }

    void VulkanSwapChain::CleanupFramebufferImages()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        // vkDestroyImageView(device.Device(), m_ColorImageView, nullptr);
        // vkDestroyImageView(device.Device(), m_DepthImageView, nullptr);

        // vkDestroyImage(device.Device(), m_ColorImage, nullptr);
        // vkDestroyImage(device.Device(), m_DepthImage, nullptr);

        // vkFreeMemory(device.Device(), m_ColorImageMemory, nullptr);
        // vkFreeMemory(device.Device(), m_DepthImageMemory, nullptr);
    }

    void VulkanSwapChain::CreateFramebuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        m_SwapChainData.Framebuffers.resize(m_SwapChainData.ImageViews.size());
        for (int i = 0; i < m_SwapChainData.ImageViews.size(); i++)
        {
            //std::array<VkImageView, 3> attachments = { m_ColorImageView, m_SwapChainData.ImageViews[i], m_DepthImageView };
            std::array<VkImageView, 1> attachments = { m_SwapChainData.ImageViews[i] };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_SwapChainData.Extent.width;
            framebufferInfo.height = m_SwapChainData.Extent.height;
            framebufferInfo.layers = 1;
            
            HE_VULKAN_CHECK_RESULT(vkCreateFramebuffer(device.Device(), &framebufferInfo, nullptr, &m_SwapChainData.Framebuffers[i]));
        }
    }

    void VulkanSwapChain::CleanupFramebuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        for (int i = 0; i < m_SwapChainData.Framebuffers.size(); i++)
        {
            vkDestroyFramebuffer(device.Device(), m_SwapChainData.Framebuffers[i], nullptr);
        }
    }

    void VulkanSwapChain::AllocateCommandBuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        // primary
        m_SwapChainData.CommandBuffers.resize(m_SwapChainData.Framebuffers.size());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VulkanContext::GetGraphicsPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<u32>(m_SwapChainData.CommandBuffers.size());

        HE_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device.Device(), &allocInfo, m_SwapChainData.CommandBuffers.data()));

        // allocate the secondary command buffer used for rendering to the swap chain
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocInfo.commandBufferCount = static_cast<u32>(m_CommandBuffers.size());

        HE_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device.Device(), &allocInfo, m_CommandBuffers.data()));
    }

    void VulkanSwapChain::FreeCommandBuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkFreeCommandBuffers(device.Device(), VulkanContext::GetGraphicsPool(), static_cast<u32>(m_CommandBuffers.size()), m_CommandBuffers.data());
        vkFreeCommandBuffers(device.Device(), VulkanContext::GetGraphicsPool(), static_cast<u32>(m_SwapChainData.CommandBuffers.size()), m_SwapChainData.CommandBuffers.data());
    }

    void VulkanSwapChain::CreateSynchronizationObjects()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        m_ImagesInFlight.resize(m_SwapChainData.Images.size(), VK_NULL_HANDLE);
        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            HE_VULKAN_CHECK_RESULT(vkCreateSemaphore(device.Device(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]));
            HE_VULKAN_CHECK_RESULT(vkCreateSemaphore(device.Device(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]));
            HE_VULKAN_CHECK_RESULT(vkCreateFence(device.Device(), &fenceInfo, nullptr, &m_InFlightFences[i]));
            HE_VULKAN_CHECK_RESULT(vkCreateFence(device.Device(), &fenceInfo, nullptr, &m_InFlightTransferFences[i]));
            HE_VULKAN_CHECK_RESULT(vkCreateFence(device.Device(), &fenceInfo, nullptr, &m_InFlightComputeFences[i]));
        }
    }

    void VulkanSwapChain::CleanupSynchronizationObjects()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device.Device(), m_ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device.Device(), m_RenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device.Device(), m_InFlightFences[i], nullptr);
            vkDestroyFence(device.Device(), m_InFlightTransferFences[i], nullptr);
            vkDestroyFence(device.Device(), m_InFlightComputeFences[i], nullptr);
        }

        for (auto semaphore : m_AuxiliaryRenderFinishedSemaphores)
            vkDestroySemaphore(device.Device(), semaphore, nullptr);
    }

    VkSemaphore VulkanSwapChain::GetAuxiliaryRenderSemaphore(size_t renderIndex)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        size_t arrayIndex = renderIndex * MAX_FRAMES_IN_FLIGHT + m_InFlightFrameIndex;
        while (arrayIndex >= m_AuxiliaryRenderFinishedSemaphores.size())
        {
            VkSemaphore semaphore;
            HE_VULKAN_CHECK_RESULT(vkCreateSemaphore(device.Device(), &semaphoreInfo, nullptr, &semaphore));
            m_AuxiliaryRenderFinishedSemaphores.push_back(semaphore);
        }

        return m_AuxiliaryRenderFinishedSemaphores[arrayIndex];
    }

    VkSemaphore VulkanSwapChain::GetAuxiliaryComputeSemaphore(size_t renderIndex)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        size_t arrayIndex = renderIndex * MAX_FRAMES_IN_FLIGHT + m_InFlightFrameIndex;
        while (arrayIndex >= m_AuxiliaryComputeFinishedSemaphores.size())
        {
            VkSemaphore semaphore;
            HE_VULKAN_CHECK_RESULT(vkCreateSemaphore(device.Device(), &semaphoreInfo, nullptr, &semaphore));
            m_AuxiliaryComputeFinishedSemaphores.push_back(semaphore);
        }

        return m_AuxiliaryComputeFinishedSemaphores[arrayIndex];
    }

    void VulkanSwapChain::RecreateSwapChain()
    {
        HE_ENGINE_LOG_TRACE("Recreating vulkan swap chain");

        VulkanDevice& device = VulkanContext::GetDevice();

        vkDeviceWaitIdle(device.Device());

        CleanupFramebuffers();

        CleanupFramebufferImages();

        CleanupSwapChain();

        CreateSwapChain();

        CreateFramebufferImages();

        CreateFramebuffers();

        m_ImagesInFlight.resize(m_SwapChainData.Images.size(), VK_NULL_HANDLE);
    }

    void VulkanSwapChain::BeginFrame()
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("VulkanSwapChain::BeginFrame");

        VulkanDevice& device = VulkanContext::GetDevice();

        // get the next image we should present to
        VkResult result = vkAcquireNextImageKHR(device.Device(), m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_InFlightFrameIndex], VK_NULL_HANDLE, &m_PresentImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
            m_ShouldPresentThisFrame = false;
        else
        {
            HE_ENGINE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);
            m_ShouldPresentThisFrame = true;
        }

        // wait for this frame image to complete
        vkWaitForFences(device.Device(), 1, &m_InFlightFences[m_InFlightFrameIndex], VK_TRUE, UINT64_MAX);

        // wait for any remaining transfers to complete
        vkWaitForFences(device.Device(), 1, &m_InFlightTransferFences[m_InFlightFrameIndex], VK_TRUE, UINT64_MAX);

        // wait for any remaining compute shaders to complete
        vkWaitForFences(device.Device(), 1, &m_InFlightComputeFences[m_InFlightFrameIndex], VK_TRUE, UINT64_MAX);

        // check if a previous frame is using this image (i.e. there is its fence to wait on)
        if (m_ImagesInFlight[m_PresentImageIndex] != VK_NULL_HANDLE)
            vkWaitForFences(device.Device(), 1, &m_ImagesInFlight[m_PresentImageIndex], VK_TRUE, UINT64_MAX);

        // TODO: look at this
        m_ImagesInFlight[m_PresentImageIndex] = m_InFlightFences[m_InFlightFrameIndex];

        // start recording the main secondary command buffer for rendering directly to the screen
        VkCommandBufferInheritanceInfo inheritanceInfo{};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.pNext = nullptr;
        inheritanceInfo.renderPass = m_RenderPass;
        inheritanceInfo.subpass = 0;
        inheritanceInfo.framebuffer = VK_NULL_HANDLE;

        VkCommandBufferBeginInfo secondaryBeginInfo{};
        secondaryBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        secondaryBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        secondaryBeginInfo.pInheritanceInfo = &inheritanceInfo;

        HE_VULKAN_CHECK_RESULT(vkBeginCommandBuffer(GetCommandBuffer(), &secondaryBeginInfo));

        // TODO: paramaterize / generalize
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (f32)m_SwapChainData.Extent.width;
        viewport.height = (f32)m_SwapChainData.Extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(GetCommandBuffer(), 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_SwapChainData.Extent;
        vkCmdSetScissor(GetCommandBuffer(), 0, 1, &scissor);
    }

    void VulkanSwapChain::EndFrame()
    {
        HE_PROFILE_FUNCTION();

        // end the main secondary command buffer for this frame
        HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(GetCommandBuffer()));

        // start swapchain image primary command buffer and execute the recorded commands in m_CommandBuffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        HE_VULKAN_CHECK_RESULT(vkBeginCommandBuffer(m_SwapChainData.CommandBuffers[m_PresentImageIndex], &beginInfo));

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_SwapChainData.Framebuffers[m_PresentImageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_SwapChainData.Extent;

        std::array<VkClearValue, 3> clearValues{};
        clearValues[0].color = { m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a };
        clearValues[1].color = { m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a };
        clearValues[2].depthStencil = { 1.f, 0 };
        renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_SwapChainData.CommandBuffers[m_PresentImageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    
        vkCmdExecuteCommands(m_SwapChainData.CommandBuffers[m_PresentImageIndex], 1, &m_CommandBuffers[m_InFlightFrameIndex]);

        vkCmdEndRenderPass(m_SwapChainData.CommandBuffers[m_PresentImageIndex]);
        
        HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(m_SwapChainData.CommandBuffers[m_PresentImageIndex]));

        if (m_ShouldPresentThisFrame)
            Present();
        else
            RecreateSwapChain();

        m_InFlightFrameIndex = (m_InFlightFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

        // clear the submitted command buffers
        m_SubmittedCommandBuffers.clear();
        m_FramebufferSubmissions.clear();
    }

    void VulkanSwapChain::Present()
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("VulkanSwapChain::Present");
        
        VulkanDevice& device = VulkanContext::GetDevice();

        std::vector<VkSubmitInfo> submitInfos(m_FramebufferSubmissions.size() + 1);
        std::vector<VkSubmitInfo> transferSubmitInfos;
        std::vector<VkSubmitInfo> computeSubmitInfos;
        std::vector<VkSemaphore> auxDrawSemaphores(m_FramebufferSubmissions.size());
        std::vector<VkSemaphore> auxCompSemaphores(m_FramebufferSubmissions.size() * 2);
        VkPipelineStageFlags drawWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkPipelineStageFlags transferWaitStages[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };
        VkPipelineStageFlags computePreWaitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
        VkPipelineStageFlags computePostWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        for (size_t i = 0; i < m_FramebufferSubmissions.size(); i++)
        {
            auto& subData = m_FramebufferSubmissions[i];
            auxDrawSemaphores[i] = GetAuxiliaryRenderSemaphore(i);

            bool preCompute = subData.PreRenderComputeBufferCount > 0;
            bool postCompute = subData.PostRenderComputeBufferCount > 0;

            if (preCompute)
            {
                auxCompSemaphores[i * 2] = GetAuxiliaryComputeSemaphore(i * 2);

                VkSubmitInfo preRenderComputeSubmit{};
                preRenderComputeSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                preRenderComputeSubmit.pWaitDstStageMask = computePreWaitStages;
                preRenderComputeSubmit.commandBufferCount = subData.PreRenderComputeBufferCount;
                preRenderComputeSubmit.pCommandBuffers = m_SubmittedCommandBuffers.data() + subData.PreRenderComputeBufferStartIndex;
                preRenderComputeSubmit.waitSemaphoreCount = 1;
                preRenderComputeSubmit.pWaitSemaphores = i > 0 ? &auxDrawSemaphores[i - 1] : nullptr; // wait for the previous frame to finish
                preRenderComputeSubmit.signalSemaphoreCount = 1;
                preRenderComputeSubmit.pSignalSemaphores = &auxCompSemaphores[i];
                computeSubmitInfos.emplace_back(preRenderComputeSubmit);
            }

            if (postCompute)
            {
                auxCompSemaphores[i * 2 + 1] = GetAuxiliaryComputeSemaphore(i * 2 + 1);

                VkSubmitInfo postRenderComputeSubmit{};
                postRenderComputeSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                postRenderComputeSubmit.pWaitDstStageMask = computePostWaitStages;
                postRenderComputeSubmit.commandBufferCount = subData.PostRenderComputeBufferCount;
                postRenderComputeSubmit.pCommandBuffers = m_SubmittedCommandBuffers.data() + subData.PostRenderComputeBufferStartIndex;
                postRenderComputeSubmit.waitSemaphoreCount = 1;
                postRenderComputeSubmit.pWaitSemaphores = &auxCompSemaphores[i * 2 + 1];
                postRenderComputeSubmit.signalSemaphoreCount = 1;
                postRenderComputeSubmit.pSignalSemaphores = &auxDrawSemaphores[i];
                computeSubmitInfos.emplace_back(postRenderComputeSubmit);
            }

            // TODO: optimize this so each submission has its own signal semaphore
            submitInfos[i] = {};
            submitInfos[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfos[i].pWaitDstStageMask = drawWaitStages;
            submitInfos[i].commandBufferCount = subData.DrawBufferCount;
            submitInfos[i].pCommandBuffers = m_SubmittedCommandBuffers.data() + subData.DrawBufferStartIndex;
            submitInfos[i].waitSemaphoreCount = i == 0 ? 0 : 1;
            if (preCompute)
                submitInfos[i].pWaitSemaphores = &auxCompSemaphores[i];
            else
                submitInfos[i].pWaitSemaphores = i > 0 ? &auxDrawSemaphores[i - 1] : nullptr;
            submitInfos[i].signalSemaphoreCount = 1;
            if (postCompute)
                submitInfos[i].pSignalSemaphores = &auxCompSemaphores[i * 2 + 1];
            else
                submitInfos[i].pSignalSemaphores = &auxDrawSemaphores[i];

            if (subData.TransferBufferCount > 0)
            {
                VkSubmitInfo transferSubmitInfo{};
                transferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                transferSubmitInfo.pWaitDstStageMask = transferWaitStages;
                transferSubmitInfo.commandBufferCount = subData.TransferBufferCount;
                transferSubmitInfo.pCommandBuffers = m_SubmittedCommandBuffers.data() + subData.TransferBufferStartIndex;
                transferSubmitInfo.waitSemaphoreCount = 1;
                transferSubmitInfo.pWaitSemaphores = &auxDrawSemaphores[i];
                transferSubmitInfos.emplace_back(transferSubmitInfo);
            }
        }
        
        // final render submission
        VkSubmitInfo& finalSubmitInfo = submitInfos.back();
        VkSemaphore finalWaitSemaphores[] = { m_ImageAvailableSemaphores[m_InFlightFrameIndex], auxDrawSemaphores.empty() ? nullptr : auxDrawSemaphores.back() };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        finalSubmitInfo = {};
        finalSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        finalSubmitInfo.waitSemaphoreCount = auxDrawSemaphores.empty() ? 1 : 2;
        finalSubmitInfo.pWaitSemaphores = finalWaitSemaphores;
        finalSubmitInfo.pWaitDstStageMask = waitStages;
        finalSubmitInfo.commandBufferCount = 1;
        finalSubmitInfo.pCommandBuffers = &m_SwapChainData.CommandBuffers[m_PresentImageIndex];

        VkSemaphore finalSignalSemaphores[] = { m_RenderFinishedSemaphores[m_InFlightFrameIndex] };
        finalSubmitInfo.signalSemaphoreCount = 1;
        finalSubmitInfo.pSignalSemaphores = finalSignalSemaphores;

        vkResetFences(device.Device(), 1, &m_InFlightFences[m_InFlightFrameIndex]);
        HE_VULKAN_CHECK_RESULT(vkQueueSubmit(device.GraphicsQueue(), static_cast<u32>(submitInfos.size()), submitInfos.data(), m_InFlightFences[m_InFlightFrameIndex]));
        if (!transferSubmitInfos.empty())
        {
            vkResetFences(device.Device(), 1, &m_InFlightTransferFences[m_InFlightFrameIndex]);
            HE_VULKAN_CHECK_RESULT(vkQueueSubmit(device.TransferQueue(), static_cast<u32>(transferSubmitInfos.size()), transferSubmitInfos.data(), m_InFlightTransferFences[m_InFlightFrameIndex]));
        }
        if (!computeSubmitInfos.empty())
        {
            vkResetFences(device.Device(), 1, &m_InFlightComputeFences[m_InFlightFrameIndex]);
            HE_VULKAN_CHECK_RESULT(vkQueueSubmit(device.ComputeQueue(), static_cast<u32>(computeSubmitInfos.size()), computeSubmitInfos.data(), m_InFlightComputeFences[m_InFlightFrameIndex]));
        }

        VkSwapchainKHR swapChains[] = { m_SwapChain };
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = finalSignalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &m_PresentImageIndex;

        VkResult result = vkQueuePresentKHR(device.PresentQueue(), &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_SwapChainInvalid)
        {
            m_SwapChainInvalid = false;
            RecreateSwapChain();
        }
        else
            HE_ENGINE_ASSERT(result == VK_SUCCESS);

        //vkQueueWaitIdle(device.PresentQueue());
    }

    void VulkanSwapChain::SubmitCommandBuffers(const std::vector<VulkanFramebufferSubmit>& submits)
    {
        FramebufferSubmissionData subData{};
    
        subData.DrawBufferStartIndex = static_cast<u32>(m_SubmittedCommandBuffers.size());
        for (auto& submit : submits)
        {
            m_SubmittedCommandBuffers.push_back(submit.DrawBuffer);
            subData.DrawBufferCount++;
        }

        subData.TransferBufferStartIndex = static_cast<u32>(m_SubmittedCommandBuffers.size());
        for (auto& submit : submits)
        {
            if (!submit.TransferBuffer) continue;
            m_SubmittedCommandBuffers.push_back(submit.TransferBuffer);
            subData.TransferBufferCount++;
        }

        subData.PreRenderComputeBufferStartIndex = static_cast<u32>(m_SubmittedCommandBuffers.size());
        for (auto& submit : submits)
        {
            if (!submit.PreRenderComputeBuffer) continue;
            m_SubmittedCommandBuffers.push_back(submit.PreRenderComputeBuffer);
            subData.PreRenderComputeBufferCount++;
        }

        subData.PostRenderComputeBufferStartIndex = static_cast<u32>(m_SubmittedCommandBuffers.size());
        for (auto& submit : submits)
        {
            if (!submit.PostRenderComputeBuffer) continue;
            m_SubmittedCommandBuffers.push_back(submit.PostRenderComputeBuffer);
            subData.PostRenderComputeBufferCount++;
        }

        m_FramebufferSubmissions.push_back(subData);
    }

    void VulkanSwapChain::InvalidateSwapChain(u32 newWidth, u32 newHeight)
    {
        m_SwapChainInvalid = true;
        m_Width = newWidth;
        m_Height = newHeight;
    }

    VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
    {
        HE_ENGINE_ASSERT(formats.size() > 0, "Swapchain has no supported surface formats");

        std::array<VkFormat, 4> preferredFormats = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        // If only VK_FORMAT_UNDEFINED is available, we can select any format
        if (formats.size() == 1)
        {
            if (formats[0].format == VK_FORMAT_UNDEFINED)
                return { preferredFormats[0], colorSpace }; // Most preferred format
            else
                return formats[0];
        }
        else
        {
            // Search for preferred formats in order and return the first one that is available
            for (auto pf : preferredFormats)
            {
                for (auto af : formats)
                {
                    if (af.format == pf && af.colorSpace == colorSpace)
                        return af;
                }
            }

            // Return the first available
            return formats[0];
        }
    }

    VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
    {
        HE_ENGINE_ASSERT(presentModes.size() > 0, "Swapchain has no supported present modes");

        // (from ImGui) Even thought mailbox seems to get us maximum framerate with a single window, it halves framerate with a second window etc. (w/ Nvidia and SDK 1.82.1)
        std::array<VkPresentModeKHR, 3> preferredModes = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };

        // Search for preferred modes in order and return the first one that is available
        for (auto pm : preferredModes)
        {
            for (auto am : presentModes)
            {
                if (am == pm)
                    return pm;
            }
        }

        // Return the default mode
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
            return capabilities.currentExtent;
        else
        {
            VkExtent2D actualExtent = {
                static_cast<u32>(m_Width),
                static_cast<u32>(m_Height)
            };

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }
}