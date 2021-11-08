#include "htpch.h"
#include "VulkanFramebuffer.h"

#include "Heart/Core/Window.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "Heart/Platform/Vulkan/VulkanGraphicsPipeline.h"
#include "Heart/Core/App.h"
#include "Heart/Renderer/Renderer.h"
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
        std::vector<VkAttachmentDescription> attachmentDescriptions = {};

        m_DepthFormat = VK_FORMAT_D32_SFLOAT;
        m_ImageSamples = VulkanCommon::MsaaSampleCountToVulkan(createInfo.SampleCount);
        if (m_ImageSamples > device.MaxMsaaSamples())
            m_ImageSamples = device.MaxMsaaSamples();

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = VK_ATTACHMENT_UNUSED;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        if (createInfo.HasDepth)
        {
            CreateDepthAttachment();

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = m_DepthFormat;
            depthAttachment.samples = m_ImageSamples;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL; //VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachmentDescriptions.emplace_back(depthAttachment);

            VkClearValue clearValue{};
            clearValue.depthStencil = { Renderer::IsUsingReverseDepth() ? 0.f : 1.f, 0 };
            m_CachedClearValues.emplace_back(clearValue);

            depthAttachmentRef.attachment = 0;
            attachmentIndex++;
        }

        // TODO: depth resolve (https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/performance/msaa/msaa_tutorial.md)
        for (auto& attachment : createInfo.Attachments)
        {
            VkFormat colorFormat = VulkanCommon::ColorFormatToVulkan(attachment.Format);
            VkClearValue clearValue{};
            clearValue.color = { attachment.ClearColor.r, attachment.ClearColor.g, attachment.ClearColor.b, attachment.ClearColor.a };

            VulkanFramebufferAttachment attachmentData = {};
            attachmentData.GeneralColorFormat = attachment.Format;
            attachmentData.ColorFormat = colorFormat;
            attachmentData.HasResolve = m_ImageSamples > VK_SAMPLE_COUNT_1_BIT;
            attachmentData.CPUVisible = attachment.AllowCPURead;

            CreateAttachmentImages(attachmentData, colorFormat);

            // create the associated renderpass
            // TODO: adjustable depth precision
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = colorFormat;
            colorAttachment.samples = m_ImageSamples;
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

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = attachmentIndex++;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentRefs.emplace_back(colorAttachmentRef);

            VkAttachmentReference colorAttachmentResolveRef{};
            colorAttachmentResolveRef.attachment = attachmentData.HasResolve ? attachmentIndex++ : VK_ATTACHMENT_UNUSED;
            colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveAttachmentRefs.emplace_back(colorAttachmentResolveRef);

            m_AttachmentData.emplace_back(attachmentData);
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<u32>(colorAttachmentRefs.size());
        subpass.pColorAttachments = colorAttachmentRefs.data();
        subpass.pResolveAttachments = resolveAttachmentRefs.data();
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

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

        if (m_Info.HasDepth)
            CleanupDepthAttachment();

        vkDestroyRenderPass(device.Device(), m_RenderPass, nullptr);

        FreeCommandBuffers();
    }

    void VulkanFramebuffer::Bind()
    {
        HE_PROFILE_FUNCTION();

        UpdateFrameIndex();

        HE_ENGINE_ASSERT(!m_SubmittedThisFrame, "Cannot bind a framebuffer that has already been submitted");

        VkCommandBuffer buffer = GetCommandBuffer();
        VulkanContext::SetBoundCommandBuffer(buffer);

        if (!m_BoundThisFrame)
        {
            m_BoundThisFrame = true;
            if (!m_Valid)
                Recreate();
     
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
    }

    void VulkanFramebuffer::Submit()
    {
        HE_PROFILE_FUNCTION();

        HE_ENGINE_ASSERT(!m_SubmittedThisFrame, "Cannot submit framebuffer twice in the same frame");
        HE_ENGINE_ASSERT(m_BoundThisFrame, "Cannot submit framebuffer that has not been bound this frame");

        VkCommandBuffer buffer = GetCommandBuffer();

        vkCmdEndRenderPass(buffer);

        // copy all CPU visible attachments to their respective buffers
        VkBufferImageCopy copyData{};
        copyData.bufferOffset = 0;
        copyData.imageSubresource.baseArrayLayer = 0;
        copyData.imageSubresource.layerCount = 1;
        copyData.imageSubresource.mipLevel = 0;
        copyData.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyData.imageExtent = { m_ActualWidth, m_ActualHeight, 1 };
        
        for (size_t i = 0; i < m_AttachmentData.size(); i++)
        {
            if (!m_AttachmentData[i].CPUVisible)
                continue;

            VkImage image = m_AttachmentData[i].ColorImage;
            if (m_AttachmentData[i].HasResolve)
                image = m_AttachmentData[i].ResolveImage;

            VulkanCommon::TransitionImageLayout(VulkanContext::GetDevice().Device(), buffer, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            vkCmdCopyImageToBuffer(buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ((VulkanBuffer*)m_AttachmentData[i].AttachmentBuffer.get())->GetBuffer(), 1, &copyData);
            VulkanCommon::TransitionImageLayout(VulkanContext::GetDevice().Device(), buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(buffer));

        m_BoundPipeline = "";
        m_BoundThisFrame = false;
        m_SubmittedThisFrame = true;
    }

    void VulkanFramebuffer::BindPipeline(const std::string& name)
    {
        HE_PROFILE_FUNCTION();

        VkCommandBuffer buffer = GetCommandBuffer();
        auto pipeline = static_cast<VulkanGraphicsPipeline*>(LoadPipeline(name).get());

        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

        m_BoundPipeline = name;
    }

    void VulkanFramebuffer::BindShaderBufferResource(u32 bindingIndex, u32 elementOffset, Buffer* buffer)
    {
        HE_PROFILE_FUNCTION();

        BindShaderResource(bindingIndex, ShaderResourceType::UniformBuffer, buffer, buffer->GetLayout().GetStride() * elementOffset); // uniform vs structured buffer are the doesn't matter here
    }

    void VulkanFramebuffer::BindShaderTextureResource(u32 bindingIndex, Texture* texture)
    {
        HE_PROFILE_FUNCTION();
        
        BindShaderResource(bindingIndex, ShaderResourceType::Texture, texture, 0);
    }

    void VulkanFramebuffer::BindShaderResource(u32 bindingIndex, ShaderResourceType resourceType, void* resource, u32 offset)
    {
        // TODO: don't use string here
        HE_ENGINE_ASSERT(!m_BoundPipeline.empty(), "Must call BindPipeline before BindShaderResource");

        VulkanGraphicsPipeline& boundPipeline = static_cast<VulkanGraphicsPipeline&>(*LoadPipeline(m_BoundPipeline));
        VulkanDescriptorSet& descriptorSet = boundPipeline.GetVulkanDescriptorSet();

        if (resourceType == ShaderResourceType::UniformBuffer || resourceType == ShaderResourceType::StorageBuffer)
            descriptorSet.UpdateDynamicOffset(bindingIndex, offset);

        // we only want to bind here if the descriptor set was allocated & updated
        // this will only occur if the binding resource differs at the same bind index and
        // only once all bind indexes have been bound with a resource this frame 
        if (descriptorSet.UpdateShaderResource(bindingIndex, resourceType, resource))
        {
            HE_ENGINE_ASSERT(descriptorSet.GetMostRecentDescriptorSet() != nullptr);

            VkDescriptorSet sets[1] = { descriptorSet.GetMostRecentDescriptorSet() };
            vkCmdBindDescriptorSets(
                GetCommandBuffer(),
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                boundPipeline.GetLayout(),
                0, 1,
                sets,
                static_cast<u32>(descriptorSet.GetDynamicOffsets().size()),
                descriptorSet.GetDynamicOffsets().data()
            );
        }
    }
    
    void* VulkanFramebuffer::GetColorAttachmentImGuiHandle(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_AttachmentData.size(), "Attachment access on framebuffer out of range");
 
        return (m_AttachmentData[attachmentIndex].HasResolve ? m_AttachmentData[attachmentIndex].ResolveImageImGuiId : m_AttachmentData[attachmentIndex].ColorImageImGuiId);
    }

    void* VulkanFramebuffer::GetDepthAttachmentImGuiHandle()
    {
        HE_ENGINE_ASSERT(m_Info.HasDepth, "Cannot get framebuffer depth attachment handle, HasDepth = false");
        HE_ENGINE_ASSERT(m_Info.SampleCount == MsaaSampleCount::None, "Cannot get framebuffer depth attachment handle, SampleCount != None");
        
        return m_DepthImageImGuiId;
    }

    void* VulkanFramebuffer::GetAttachmentPixelData(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_AttachmentData.size(), "Attachment access on framebuffer out of range");
        HE_ENGINE_ASSERT(m_AttachmentData[attachmentIndex].CPUVisible, "Cannot read pixel data of attachment that does not have 'AllowCPURead' enabled");

        return ((VulkanBuffer*)m_AttachmentData[attachmentIndex].AttachmentBuffer.get())->GetMappedMemory();
    }

    Ref<GraphicsPipeline> VulkanFramebuffer::InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.BlendStates.size() == m_Info.Attachments.size(), "Graphics pipeline blend state count must match framebuffer attachment count");

        VulkanDevice& device = VulkanContext::GetDevice();

        return CreateRef<VulkanGraphicsPipeline>(createInfo, m_RenderPass, m_ImageSamples, m_Info.Width, m_Info.Height);
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

    void VulkanFramebuffer::CreateAttachmentImages(VulkanFramebufferAttachment& attachmentData, VkFormat colorFormat)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        bool colorTransferSrc = attachmentData.CPUVisible && !attachmentData.HasResolve;

        // create the associated images for the framebuffer
        VulkanCommon::CreateImage(
            device.Device(),
            device.PhysicalDevice(),
            m_ActualWidth,
            m_ActualHeight,
            colorFormat,
            1,
            m_ImageSamples,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | (colorTransferSrc ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
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
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | (attachmentData.CPUVisible ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                attachmentData.ResolveImage,
                attachmentData.ResolveImageMemory
            );
        }

        if (attachmentData.CPUVisible)
            attachmentData.AttachmentBuffer = VulkanBuffer::Create(Buffer::Type::None, { ColorFormatBufferDataType(attachmentData.GeneralColorFormat) }, m_ActualWidth * m_ActualHeight * ColorFormatComponents(attachmentData.GeneralColorFormat));

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

        attachmentData.AttachmentBuffer.reset();
    }

    void VulkanFramebuffer::CreateDepthAttachment()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VulkanCommon::CreateImage(
            device.Device(),
            device.PhysicalDevice(),
            m_ActualWidth,
            m_ActualHeight,
            m_DepthFormat,
            1,
            m_ImageSamples,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_DepthImage,
            m_DepthImageMemory
        );

        m_DepthImageView = VulkanCommon::CreateImageView(device.Device(), m_DepthImage, m_DepthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);
        m_CachedImageViews.emplace_back(m_DepthImageView);
        m_DepthImageImGuiId = ImGui_ImplVulkan_AddTexture(VulkanContext::GetDefaultSampler(), m_DepthImageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);
    }

    void VulkanFramebuffer::CleanupDepthAttachment()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        ImGui_ImplVulkan_RemoveTexture(m_DepthImageImGuiId);

        vkDestroyImageView(device.Device(), m_DepthImageView, nullptr);
        vkDestroyImage(device.Device(), m_DepthImage, nullptr);
        vkFreeMemory(device.Device(), m_DepthImageMemory, nullptr);
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

        if (m_Info.HasDepth)
        {
            CleanupDepthAttachment();
            CreateDepthAttachment();
        }

        for (auto& attachmentData : m_AttachmentData)
        {
            CleanupAttachmentImages(attachmentData);
            CreateAttachmentImages(attachmentData, attachmentData.ColorFormat);
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

            m_SubmittedThisFrame = false;
        }
    }
}
