#include "htpch.h"
#include "VulkanSwapChain.h"

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

        CreateFrameBufferImages();

        CreateFrameBuffers();

        AllocateCommandBuffers();

        CreateSynchronizationObjects();

        m_Initialized = true;
    }

    void VulkanSwapChain::Shutdown()
    {
        if (!m_Initialized) return;

        CleanupSynchronizationObjects();

        FreeCommandBuffers();

        CleanupFrameBuffers();

        CleanupFrameBufferImages();

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

        u32 imageCount = swapChainSupport.Capabilities.minImageCount + 1;
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
            m_SwapChainData.ImageViews[i] = VulkanCommon::CreateImageView(device.Device(), m_SwapChainData.Images[i], m_SwapChainData.ImageFormat, 1);
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
        // TODO: variable samplecount
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = device.MaxMsaaSamples();
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_SwapChainData.ImageFormat;
        colorAttachment.samples = device.MaxMsaaSamples();
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = m_SwapChainData.ImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 1;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 2;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, colorAttachmentResolve, depthAttachment };
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

    void VulkanSwapChain::CreateFrameBufferImages()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkFormat colorFormat = m_SwapChainData.ImageFormat;
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

        // TODO: variable samplecount
        VulkanCommon::CreateImage(
            device.Device(),
            device.PhysicalDevice(),
            m_SwapChainData.Extent.width,
            m_SwapChainData.Extent.height,
            colorFormat,
            1,
            device.MaxMsaaSamples(), 
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_ColorImage,
            m_ColorImageMemory
        );
        m_ColorImageView = VulkanCommon::CreateImageView(device.Device(), m_ColorImage, colorFormat, 1);

        VulkanCommon::CreateImage(
            device.Device(),
            device.PhysicalDevice(),
            m_SwapChainData.Extent.width,
            m_SwapChainData.Extent.height,
            depthFormat,
            1,
            device.MaxMsaaSamples(),
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_DepthImage,
            m_DepthImageMemory
        );
        m_DepthImageView = VulkanCommon::CreateImageView(device.Device(), m_DepthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);      
    }

    void VulkanSwapChain::CleanupFrameBufferImages()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkDestroyImageView(device.Device(), m_ColorImageView, nullptr);
        vkDestroyImageView(device.Device(), m_DepthImageView, nullptr);

        vkDestroyImage(device.Device(), m_ColorImage, nullptr);
        vkDestroyImage(device.Device(), m_DepthImage, nullptr);

        vkFreeMemory(device.Device(), m_ColorImageMemory, nullptr);
        vkFreeMemory(device.Device(), m_DepthImageMemory, nullptr);
    }

    void VulkanSwapChain::CreateFrameBuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        m_SwapChainData.FrameBuffers.resize(m_SwapChainData.ImageViews.size());
        for (int i = 0; i < m_SwapChainData.ImageViews.size(); i++)
        {
            std::array<VkImageView, 3> attachments = { m_ColorImageView, m_SwapChainData.ImageViews[i], m_DepthImageView };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_SwapChainData.Extent.width;
            framebufferInfo.height = m_SwapChainData.Extent.height;
            framebufferInfo.layers = 1;
            
            HE_VULKAN_CHECK_RESULT(vkCreateFramebuffer(device.Device(), &framebufferInfo, nullptr, &m_SwapChainData.FrameBuffers[i]));
        }
    }

    void VulkanSwapChain::CleanupFrameBuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        for (int i = 0; i < m_SwapChainData.FrameBuffers.size(); i++)
        {
            vkDestroyFramebuffer(device.Device(), m_SwapChainData.FrameBuffers[i], nullptr);
        }
    }

    void VulkanSwapChain::AllocateCommandBuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        // primary
        m_SwapChainData.CommandBuffers.resize(m_SwapChainData.FrameBuffers.size());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VulkanContext::GetGraphicsPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<u32>(m_SwapChainData.CommandBuffers.size());

        HE_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device.Device(), &allocInfo, m_SwapChainData.CommandBuffers.data()));

        // allocate the secondary command buffer used for rendering to the swap chain
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

        m_CommandBuffers.resize(m_SwapChainData.CommandBuffers.size());
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

        m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight);
        m_RenderFinishedSemaphores.resize(m_MaxFramesInFlight);
        m_AuxiliaryRenderFinishedSemaphores.resize(m_MaxFramesInFlight);
        m_InFlightFences.resize(m_MaxFramesInFlight);
        m_ImagesInFlight.resize(m_SwapChainData.Images.size(), VK_NULL_HANDLE);
        for (u32 i = 0; i < m_MaxFramesInFlight; i++)
        {
            HE_VULKAN_CHECK_RESULT(vkCreateSemaphore(device.Device(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]));
            HE_VULKAN_CHECK_RESULT(vkCreateSemaphore(device.Device(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]));
            HE_VULKAN_CHECK_RESULT(vkCreateSemaphore(device.Device(), &semaphoreInfo, nullptr, &m_AuxiliaryRenderFinishedSemaphores[i]));
            HE_VULKAN_CHECK_RESULT(vkCreateFence(device.Device(), &fenceInfo, nullptr, &m_InFlightFences[i]));
        }
    }

    void VulkanSwapChain::CleanupSynchronizationObjects()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        for (u32 i = 0; i < m_MaxFramesInFlight; i++)
        {
            vkDestroySemaphore(device.Device(), m_ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device.Device(), m_RenderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device.Device(), m_AuxiliaryRenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device.Device(), m_InFlightFences[i], nullptr);
        }
    }

    void VulkanSwapChain::RecreateSwapChain()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkDeviceWaitIdle(device.Device());

        //FreeCommandBuffers();

        CleanupFrameBuffers();

        CleanupFrameBufferImages();

        CleanupSwapChain();

        CreateSwapChain();

        CreateFrameBufferImages();

        CreateFrameBuffers();

        // TODO: look into this
        // we might not need to reallocate here since the only reason we would need to is if the number if swapchain images changes (which it shouldnt)
        //AllocateCommandBuffers();
    }

    void VulkanSwapChain::BeginFrame()
    {
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

        // clear the submitted command buffers from last frame
        m_AuxiliaryCommandBuffers.clear();
    }

    void VulkanSwapChain::EndFrame()
    {
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
        renderPassInfo.framebuffer = m_SwapChainData.FrameBuffers[m_PresentImageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_SwapChainData.Extent;

        std::array<VkClearValue, 3> clearValues{};
        clearValues[0].color = { m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a };
        clearValues[1].color = { m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a };
        clearValues[2].depthStencil = { 1.f, 0 };
        renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_SwapChainData.CommandBuffers[m_PresentImageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    
        vkCmdExecuteCommands(m_SwapChainData.CommandBuffers[m_PresentImageIndex], 1, &m_CommandBuffers[m_PresentImageIndex]);

        vkCmdEndRenderPass(m_SwapChainData.CommandBuffers[m_PresentImageIndex]);
        
        HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(m_SwapChainData.CommandBuffers[m_PresentImageIndex]));

        if (m_ShouldPresentThisFrame)
            Present();
        else
            RecreateSwapChain();

        m_InFlightFrameIndex = (m_InFlightFrameIndex + 1) % m_MaxFramesInFlight;
    }

    void VulkanSwapChain::Present()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkWaitForFences(device.Device(), 1, &m_InFlightFences[m_InFlightFrameIndex], VK_TRUE, UINT64_MAX);

        // check if a previous frame is using this image (i.e. there is its fence to wait on)
        if (m_ImagesInFlight[m_PresentImageIndex] != VK_NULL_HANDLE)
            vkWaitForFences(device.Device(), 1, &m_ImagesInFlight[m_PresentImageIndex], VK_TRUE, UINT64_MAX);

        // TODO: look at this
        m_ImagesInFlight[m_PresentImageIndex] = m_InFlightFences[m_InFlightFrameIndex];

        //UpdatePerFrameBuffer(nextImageIndex);

        std::array<VkSubmitInfo, 2> submitInfos;
        // auxiliary framebuffer submissions
        VkPipelineStageFlags auxWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfos[0] = {};
        submitInfos[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfos[0].waitSemaphoreCount = 0;
        submitInfos[0].pWaitSemaphores = nullptr;
        submitInfos[0].pWaitDstStageMask = auxWaitStages;
        submitInfos[0].commandBufferCount = static_cast<u32>(m_AuxiliaryCommandBuffers.size());
        submitInfos[0].pCommandBuffers = m_AuxiliaryCommandBuffers.data();

        VkSemaphore auxiliarySignalSemaphores[] = { m_AuxiliaryRenderFinishedSemaphores[m_InFlightFrameIndex] };
        submitInfos[0].signalSemaphoreCount = 1;
        submitInfos[0].pSignalSemaphores = auxiliarySignalSemaphores;
        
        // final render submission
        VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_InFlightFrameIndex], m_AuxiliaryRenderFinishedSemaphores[m_InFlightFrameIndex] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfos[1] = {};
        submitInfos[1].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfos[1].waitSemaphoreCount = 2;
        submitInfos[1].pWaitSemaphores = waitSemaphores;
        submitInfos[1].pWaitDstStageMask = waitStages;
        submitInfos[1].commandBufferCount = 1;
        submitInfos[1].pCommandBuffers = &m_SwapChainData.CommandBuffers[m_PresentImageIndex];

        VkSemaphore finalSignalSemaphores[] = { m_RenderFinishedSemaphores[m_InFlightFrameIndex] };
        submitInfos[1].signalSemaphoreCount = 1;
        submitInfos[1].pSignalSemaphores = finalSignalSemaphores;

        vkResetFences(device.Device(), 1, &m_InFlightFences[m_InFlightFrameIndex]);
        HE_VULKAN_CHECK_RESULT(vkQueueSubmit(device.GraphicsQueue(), static_cast<u32>(submitInfos.size()), submitInfos.data(), m_InFlightFences[m_InFlightFrameIndex]));

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

    void VulkanSwapChain::InvalidateSwapChain(u32 newWidth, u32 newHeight)
    {
        m_SwapChainInvalid = true;
        m_Width = newWidth;
        m_Height = newHeight;
    }

    VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
    {
        for (const auto& availableFormat : formats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return availableFormat;
        }

        return formats[0];
    }

    VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
    {
        for (const auto& availablePresentMode : presentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return availablePresentMode;
        }

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