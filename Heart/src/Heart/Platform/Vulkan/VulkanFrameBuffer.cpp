#include "htpch.h"
#include "VulkanFramebuffer.h"

#include "Heart/Core/Window.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "Heart/Platform/Vulkan/VulkanGraphicsPipeline.h"
#include "Heart/Core/App.h"
#include "imgui/backends/imgui_impl_vulkan.h"

namespace Heart
{
    VulkanFramebuffer::VulkanFramebuffer(const FramebufferCreateInfo& createInfo)
        : Framebuffer(createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.Attachments.size() > 0, "Cannot create a framebuffer with zero attachments");

        VulkanDevice& device = VulkanContext::GetDevice();
        Window& mainWindow = Window::GetMainWindow();

        m_ActualWidth = createInfo.Width == 0 ? mainWindow.GetWidth() : createInfo.Width;
        m_ActualHeight = createInfo.Height == 0 ? mainWindow.GetHeight() : createInfo.Height;

        AllocateCommandBuffers();

        u32 attachmentIndex = 0;
        std::vector<VkAttachmentReference> colorAttachmentRefs = {};
        std::vector<VkAttachmentReference> resolveAttachmentRefs = {};
        std::vector<VkAttachmentReference> depthAttachmentRefs = {};
        std::vector<VkAttachmentDescription> attachmentDescriptions = {};

        VkSampleCountFlagBits imageSamples = VulkanCommon::MsaaSampleCountToVulkan(createInfo.SampleCount);
        if (imageSamples > device.MaxMsaaSamples())
            imageSamples = device.MaxMsaaSamples();

        for (auto& attachment : createInfo.Attachments)
        {
            VkFormat colorFormat = VulkanCommon::ColorFormatToVulkan(attachment.ColorFormat);
            VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
            VkClearValue clearValue{};
            clearValue.color = { attachment.ClearColor.r, attachment.ClearColor.g, attachment.ClearColor.b, attachment.ClearColor.a };

            VulkanFramebufferAttachment attachmentData = {};
            attachmentData.ColorFormat = colorFormat;
            attachmentData.DepthFormat = depthFormat;
            attachmentData.HasResolve = imageSamples > VK_SAMPLE_COUNT_1_BIT;
            attachmentData.HasDepth = attachment.HasDepth;

            CreateAttachmentImages(attachmentData, colorFormat, depthFormat);

            // create the associated renderpass
            // TODO: adjustable depth precision
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = colorFormat;
            colorAttachment.samples = imageSamples;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = (attachmentData.HasResolve ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE);
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentDescriptions.emplace_back(colorAttachment);

            m_CachedClearValues.emplace_back(clearValue);

            if (attachmentData.HasResolve)
            {
                VkAttachmentDescription colorAttachmentResolve{};
                colorAttachmentResolve.format = colorFormat;
                colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachmentDescriptions.emplace_back(colorAttachmentResolve);

                m_CachedClearValues.emplace_back(clearValue);
            }

            if (attachmentData.HasDepth)
            {
                VkAttachmentDescription depthAttachment{};
                depthAttachment.format = depthFormat;
                depthAttachment.samples = imageSamples;
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL; //VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                attachmentDescriptions.emplace_back(depthAttachment);

                clearValue.depthStencil = { 1.f, 0 };
                m_CachedClearValues.emplace_back(clearValue);
            }

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = attachmentIndex++;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentRefs.emplace_back(colorAttachmentRef);

            VkAttachmentReference colorAttachmentResolveRef{};
            colorAttachmentResolveRef.attachment = attachmentData.HasResolve ? attachmentIndex++ : VK_ATTACHMENT_UNUSED;
            colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveAttachmentRefs.emplace_back(colorAttachmentResolveRef);

            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = attachmentData.HasDepth ? attachmentIndex++ : VK_ATTACHMENT_UNUSED;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachmentRefs.emplace_back(depthAttachmentRef);

            m_AttachmentData.emplace_back(attachmentData);
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<u32>(colorAttachmentRefs.size());
        subpass.pColorAttachments = colorAttachmentRefs.data();
        subpass.pResolveAttachments = resolveAttachmentRefs.data();
        subpass.pDepthStencilAttachment = depthAttachmentRefs.data();

        // TODO: subpass dependencies
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<u32>(attachmentDescriptions.size());
        renderPassInfo.pAttachments = attachmentDescriptions.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        HE_VULKAN_CHECK_RESULT(vkCreateRenderPass(device.Device(), &renderPassInfo, nullptr, &m_RenderPass));

        CreateFramebuffer();
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext::Sync();

        CleanupFramebuffer();

        for (auto& attachmentData : m_AttachmentData)
        {
            CleanupAttachmentImages(attachmentData);
        }

        vkDestroyRenderPass(device.Device(), m_RenderPass, nullptr);

        FreeCommandBuffers();
    }

    void VulkanFramebuffer::Bind()
    {
        UpdateFrameIndex();

        if (!m_Valid)
            Recreate();

        VkCommandBuffer buffer = GetCommandBuffer();

        VulkanContext::SetBoundCommandBuffer(buffer);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        HE_VULKAN_CHECK_RESULT(vkBeginCommandBuffer(buffer, &beginInfo));

        // TODO: paramaterize / generalize
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (f32)m_ActualWidth;
        viewport.height = (f32)m_ActualHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(buffer, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { m_ActualWidth, m_ActualHeight };
        vkCmdSetScissor(buffer, 0, 1, &scissor);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_Framebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { m_ActualWidth, m_ActualHeight };

        renderPassInfo.clearValueCount = static_cast<u32>(m_CachedClearValues.size());
        renderPassInfo.pClearValues = m_CachedClearValues.data();

        vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanFramebuffer::Submit(GraphicsContext& _context)
    {
        VkCommandBuffer buffer = GetCommandBuffer();
        VulkanContext& context = static_cast<VulkanContext&>(_context);

        vkCmdEndRenderPass(buffer);
        
        HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(buffer));

        context.GetSwapChain().SubmitCommandBuffer(buffer);

        m_BoundPipeline = "";
    }

    void VulkanFramebuffer::BindPipeline(const std::string& name)
    {
        VkCommandBuffer buffer = GetCommandBuffer();
        auto pipeline = static_cast<VulkanGraphicsPipeline*>(LoadPipeline(name).get());

        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

        m_BoundPipeline = name;
    }

    void VulkanFramebuffer::BindShaderInputSet(ShaderInputBindPoint set, u32 setIndex)
    {
        // TODO: don't use string here
        HE_ENGINE_ASSERT(m_BoundPipeline != "", "Must call BindPipeline before BindShaderInputSets");

        VulkanGraphicsPipeline& boundPipeline = static_cast<VulkanGraphicsPipeline&>(*LoadPipeline(m_BoundPipeline));

        vkCmdBindDescriptorSets(GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, boundPipeline.GetLayout(), setIndex, 1, &static_cast<VkDescriptorSet>(set), 0, nullptr);
    }

    void* VulkanFramebuffer::GetRawAttachmentImageHandle(u32 attachmentIndex, FramebufferAttachmentType type)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_AttachmentData.size(), "Attachment access on framebuffer out of range");

        // TODO: depth resolve (https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/performance/msaa/msaa_tutorial.md)
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT("Cannot get framebuffer image handle unsupported FramebufferAttachmentType") } break;
            case FramebufferAttachmentType::Color:
            { return (m_AttachmentData[attachmentIndex].HasResolve ? m_AttachmentData[attachmentIndex].ResolveImageImGuiId : m_AttachmentData[attachmentIndex].ColorImageImGuiId); }
            case FramebufferAttachmentType::Depth:
            { HE_ENGINE_ASSERT(m_Info.SampleCount == MsaaSampleCount::None, "Cannot use a multisampled depth buffer as a texture"); return m_AttachmentData[attachmentIndex].DepthImageImGuiId; }
        }

        return nullptr;
    }

    Ref<GraphicsPipeline> VulkanFramebuffer::InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.BlendStates.size() == m_Info.Attachments.size(), "Graphics pipeline blend state count must match framebuffer attachment count");

        VulkanDevice& device = VulkanContext::GetDevice();

        VkSampleCountFlagBits sampleCount = VulkanCommon::MsaaSampleCountToVulkan(m_Info.SampleCount);
        if (sampleCount > device.MaxMsaaSamples())
            sampleCount = device.MaxMsaaSamples();

        return CreateRef<VulkanGraphicsPipeline>(createInfo, m_RenderPass, sampleCount, m_Info.Width, m_Info.Height);
    }

    void VulkanFramebuffer::AllocateCommandBuffers()
    {
        // get the main context instance here because we need to sync with the swapchain image count
        VulkanContext& mainContext = static_cast<VulkanContext&>(Window::GetMainWindow().GetContext());
        VulkanDevice& device = VulkanContext::GetDevice();

        // initialize the main command buffer for this framebuffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VulkanContext::GetGraphicsPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<u32>(m_CommandBuffers.size());

        HE_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device.Device(), &allocInfo, m_CommandBuffers.data()));
    }

    void VulkanFramebuffer::FreeCommandBuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkFreeCommandBuffers(device.Device(), VulkanContext::GetGraphicsPool(), static_cast<u32>(m_CommandBuffers.size()), m_CommandBuffers.data());
    }

    void VulkanFramebuffer::CreateAttachmentImages(VulkanFramebufferAttachment& attachmentData, VkFormat colorFormat, VkFormat depthFormat)
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkSampleCountFlagBits imageSamples = VulkanCommon::MsaaSampleCountToVulkan(m_Info.SampleCount);
        if (imageSamples > device.MaxMsaaSamples())
            imageSamples = device.MaxMsaaSamples();

        // create the associated images for the framebuffer
        VulkanCommon::CreateImage(
            device.Device(),
            device.PhysicalDevice(),
            m_ActualWidth,
            m_ActualHeight,
            colorFormat,
            1,
            imageSamples,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | (attachmentData.HasResolve ? VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT : 0),
            attachmentData.ColorImage,
            attachmentData.ColorImageMemory
        );

        if (attachmentData.HasResolve)
        {
            VulkanCommon::CreateImage(
                device.Device(),
                device.PhysicalDevice(),
                m_ActualWidth,
                m_ActualHeight,
                colorFormat,
                1,
                VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                attachmentData.ResolveImage,
                attachmentData.ResolveImageMemory
            );
        }

        if (attachmentData.HasDepth)
        {
            VulkanCommon::CreateImage(
                device.Device(),
                device.PhysicalDevice(),
                m_ActualWidth,
                m_ActualHeight,
                depthFormat,
                1,
                imageSamples,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
                attachmentData.DepthImage,
                attachmentData.DepthImageMemory
            );
        }

        // create associated image views 
        attachmentData.ColorImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.ColorImage, colorFormat, 1);
        m_CachedImageViews.emplace_back(attachmentData.ColorImageView);
        attachmentData.ColorImageImGuiId = ImGui_ImplVulkan_AddTexture(VulkanContext::GetDefaultSampler(), attachmentData.ColorImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

        if (attachmentData.HasResolve)
        {
            attachmentData.ResolveImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.ResolveImage, colorFormat, 1);
            m_CachedImageViews.emplace_back(attachmentData.ResolveImageView);
            attachmentData.ResolveImageImGuiId = ImGui_ImplVulkan_AddTexture(VulkanContext::GetDefaultSampler(), attachmentData.ResolveImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
        }

        if (attachmentData.HasDepth)
        {
            attachmentData.DepthImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.DepthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);
            m_CachedImageViews.emplace_back(attachmentData.DepthImageView);
            attachmentData.DepthImageImGuiId = ImGui_ImplVulkan_AddTexture(VulkanContext::GetDefaultSampler(), attachmentData.DepthImageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);
        }
    }

    void VulkanFramebuffer::CleanupAttachmentImages(VulkanFramebufferAttachment& attachmentData)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        ImGui_ImplVulkan_RemoveTexture(attachmentData.ColorImageImGuiId);
            
        vkDestroyImageView(device.Device(), attachmentData.ColorImageView, nullptr);
        vkDestroyImage(device.Device(), attachmentData.ColorImage, nullptr);
        vkFreeMemory(device.Device(), attachmentData.ColorImageMemory, nullptr);

        if (attachmentData.HasResolve)
        {
            ImGui_ImplVulkan_RemoveTexture(attachmentData.ResolveImageImGuiId);

            vkDestroyImageView(device.Device(), attachmentData.ResolveImageView, nullptr);
            vkDestroyImage(device.Device(), attachmentData.ResolveImage, nullptr);
            vkFreeMemory(device.Device(), attachmentData.ResolveImageMemory, nullptr);
        }

        if (attachmentData.HasDepth)
        {
            ImGui_ImplVulkan_RemoveTexture(attachmentData.DepthImageImGuiId);

            vkDestroyImageView(device.Device(), attachmentData.DepthImageView, nullptr);
            vkDestroyImage(device.Device(), attachmentData.DepthImage, nullptr);
            vkFreeMemory(device.Device(), attachmentData.DepthImageMemory, nullptr);
        }
    }

    void VulkanFramebuffer::CreateFramebuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = static_cast<u32>(m_CachedImageViews.size());
        framebufferInfo.pAttachments = m_CachedImageViews.data();
        framebufferInfo.width = m_ActualWidth;
        framebufferInfo.height = m_ActualHeight;
        framebufferInfo.layers = 1;
        
        HE_VULKAN_CHECK_RESULT(vkCreateFramebuffer(device.Device(), &framebufferInfo, nullptr, &m_Framebuffer));
    }

    void VulkanFramebuffer::CleanupFramebuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        
        vkDestroyFramebuffer(device.Device(), m_Framebuffer, nullptr);
    }

    void VulkanFramebuffer::Recreate()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext& mainContext = static_cast<VulkanContext&>(Window::GetMainWindow().GetContext());
        VulkanContext::Sync();

        CleanupFramebuffer();
        m_CachedImageViews.clear();

        for (auto& attachmentData : m_AttachmentData)
        {
            CleanupAttachmentImages(attachmentData);
            CreateAttachmentImages(attachmentData, attachmentData.ColorFormat, attachmentData.DepthFormat);
        }

        CreateFramebuffer();

        m_Valid = true;
    }

    void VulkanFramebuffer::UpdateFrameIndex()
    {
        if (App::Get().GetFrameCount() != m_LastUpdateFrame)
        {
            // it is a new frame so we need to get the new flight frame index
            VulkanContext& mainContext = static_cast<VulkanContext&>(Window::GetMainWindow().GetContext());

            m_InFlightFrameIndex = mainContext.GetSwapChain().GetInFlightFrameIndex();
            m_LastUpdateFrame = App::Get().GetFrameCount();
        }
    }
}
