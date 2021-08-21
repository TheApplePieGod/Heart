#include "htpch.h"
#include "VulkanFrameBuffer.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "Heart/Platform/Vulkan/VulkanGraphicsPipeline.h"
#include "imgui/backends/imgui_impl_vulkan.h"

namespace Heart
{
    VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferCreateInfo& createInfo)
        : FrameBuffer(createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.Attachments.size() > 0, "Cannot create a framebuffer with zero attachments");

        VulkanDevice& device = VulkanContext::GetDevice();

        // initialize the main command buffer for this framebuffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VulkanContext::GetGraphicsPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        HE_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device.Device(), &allocInfo, &m_CommandBuffer));

        u32 attachmentIndex = 0;
        std::vector<VkAttachmentReference> colorAttachmentRefs = {};
        std::vector<VkAttachmentReference> resolveAttachmentRefs = {};
        std::vector<VkAttachmentReference> depthAttachmentRefs = {};
        std::vector<VkAttachmentDescription> attachmentDescriptions = {};
        std::vector<VkImageView> attachmentImageViews = {};
        VkSampleCountFlagBits imageSamples = VulkanCommon::MsaaSampleCountToVulkan(createInfo.SampleCount);
        if (imageSamples > device.MaxMsaaSamples())
            imageSamples = device.MaxMsaaSamples();
        for (auto& attachment : createInfo.Attachments)
        {
            VulkanFrameBufferAttachment attachmentData = {};

            VkFormat colorFormat = VulkanCommon::ColorFormatToVulkan(attachment.ColorFormat);
            VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
            attachmentData.HasResolve = imageSamples > VK_SAMPLE_COUNT_1_BIT;
            attachmentData.HasDepth = attachment.HasDepth;
            VkClearValue clearValue{};
            clearValue.color = { attachment.ClearColor.r, attachment.ClearColor.g, attachment.ClearColor.b, attachment.ClearColor.a };

            // create the associated images for the framebuffer
            VulkanCommon::CreateImage(
                device.Device(),
                device.PhysicalDevice(),
                createInfo.Width,
                createInfo.Height,
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
                    createInfo.Width,
                    createInfo.Height,
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
                    createInfo.Width,
                    createInfo.Height,
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
            attachmentImageViews.emplace_back(attachmentData.ColorImageView);
            attachmentData.ColorImageImGuiId = ImGui_ImplVulkan_AddTexture(VulkanContext::GetDefaultSampler(), attachmentData.ColorImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

            if (attachmentData.HasResolve)
            {
                attachmentData.ResolveImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.ResolveImage, colorFormat, 1);
                attachmentImageViews.emplace_back(attachmentData.ResolveImageView);
                attachmentData.ResolveImageImGuiId = ImGui_ImplVulkan_AddTexture(VulkanContext::GetDefaultSampler(), attachmentData.ResolveImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
            }

            if (attachmentData.HasDepth)
            {
                attachmentData.DepthImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.DepthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);
                attachmentImageViews.emplace_back(attachmentData.DepthImageView);
                attachmentData.DepthImageImGuiId = ImGui_ImplVulkan_AddTexture(VulkanContext::GetDefaultSampler(), attachmentData.DepthImageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);
            }

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

        // create the final framebuffer
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = static_cast<u32>(attachmentImageViews.size());
        framebufferInfo.pAttachments = attachmentImageViews.data();
        framebufferInfo.width = createInfo.Width;
        framebufferInfo.height = createInfo.Height;
        framebufferInfo.layers = 1;
        
        HE_VULKAN_CHECK_RESULT(vkCreateFramebuffer(device.Device(), &framebufferInfo, nullptr, &m_FrameBuffer));
    }

    VulkanFrameBuffer::~VulkanFrameBuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkDestroyFramebuffer(device.Device(), m_FrameBuffer, nullptr);

        for (auto& pipeline : m_GraphicsPipelines)
        {
            pipeline.second.reset();
        }

        for (auto& attachmentData : m_AttachmentData)
        {
            vkDestroyImageView(device.Device(), attachmentData.ColorImageView, nullptr);
            vkDestroyImage(device.Device(), attachmentData.ColorImage, nullptr);
            vkFreeMemory(device.Device(), attachmentData.ColorImageMemory, nullptr);

            if (attachmentData.HasResolve)
            {
                vkDestroyImageView(device.Device(), attachmentData.ResolveImageView, nullptr);
                vkDestroyImage(device.Device(), attachmentData.ResolveImage, nullptr);
                vkFreeMemory(device.Device(), attachmentData.ResolveImageMemory, nullptr);
            }

            if (attachmentData.HasDepth)
            {
                vkDestroyImageView(device.Device(), attachmentData.DepthImageView, nullptr);
                vkDestroyImage(device.Device(), attachmentData.DepthImage, nullptr);
                vkFreeMemory(device.Device(), attachmentData.DepthImageMemory, nullptr);
            }
        }

        vkDestroyRenderPass(device.Device(), m_RenderPass, nullptr);

        vkFreeCommandBuffers(device.Device(), VulkanContext::GetGraphicsPool(), 1, &m_CommandBuffer);
    }

    void VulkanFrameBuffer::Bind()
    {
        VulkanContext::SetBoundCommandBuffer(m_CommandBuffer);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        HE_VULKAN_CHECK_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));

        // TODO: paramaterize / generalize
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (f32)m_Info.Width;
        viewport.height = (f32)m_Info.Height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = { m_Info.Width, m_Info.Height };
        vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_FrameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = VkExtent2D {m_Info.Width, m_Info.Height};

        renderPassInfo.clearValueCount = static_cast<u32>(m_CachedClearValues.size());
        renderPassInfo.pClearValues = m_CachedClearValues.data();

        vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanFrameBuffer::Submit(GraphicsContext& _context)
    {
        VulkanContext& context = static_cast<VulkanContext&>(_context);

        vkCmdEndRenderPass(m_CommandBuffer);
        
        HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(m_CommandBuffer));

        context.GetSwapChain().SubmitCommandBuffer(m_CommandBuffer);
    }

    void VulkanFrameBuffer::BindPipeline(const std::string& name)
    {
        auto pipeline = static_cast<VulkanGraphicsPipeline*>(LoadPipeline(name).get());

        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());
    }

    void* VulkanFrameBuffer::GetRawAttachmentImageHandle(u32 attachmentIndex, FrameBufferAttachmentType type)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_AttachmentData.size(), "Attachment access on framebuffer out of range");

        // TODO: depth resolve (https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/performance/msaa/msaa_tutorial.md)
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT("Cannot get framebuffer image handle unsupported FrameBufferAttachmentType") } break;
            case FrameBufferAttachmentType::Color:
            { return (m_AttachmentData[attachmentIndex].HasResolve ? m_AttachmentData[attachmentIndex].ResolveImageImGuiId : m_AttachmentData[attachmentIndex].ColorImageImGuiId); }
            case FrameBufferAttachmentType::Depth:
            { HE_ENGINE_ASSERT(m_Info.SampleCount == MsaaSampleCount::None, "Cannot use a multisampled depth buffer as a texture"); return m_AttachmentData[attachmentIndex].DepthImageImGuiId; }
        }

        return nullptr;
    }

    Ref<GraphicsPipeline> VulkanFrameBuffer::InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VkSampleCountFlagBits sampleCount = VulkanCommon::MsaaSampleCountToVulkan(m_Info.SampleCount);
        if (sampleCount > device.MaxMsaaSamples())
            sampleCount = device.MaxMsaaSamples();

        return CreateRef<VulkanGraphicsPipeline>(createInfo, m_RenderPass, sampleCount, m_Info.Width, m_Info.Height);
    }
}
