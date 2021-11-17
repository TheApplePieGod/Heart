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
        HE_ENGINE_ASSERT(createInfo.ColorAttachments.size() > 0, "Cannot create a framebuffer with zero color attachments");

        VulkanDevice& device = VulkanContext::GetDevice();
        Window& mainWindow = Window::GetMainWindow();

        m_ActualWidth = createInfo.Width == 0 ? mainWindow.GetWidth() : createInfo.Width;
        m_ActualHeight = createInfo.Height == 0 ? mainWindow.GetHeight() : createInfo.Height;

        AllocateCommandBuffers();

        u32 attachmentIndex = 0;
        std::vector<VkAttachmentReference> colorAttachmentRefs = {};
        std::vector<VkAttachmentReference> resolveAttachmentRefs = {};
        std::vector<VkAttachmentDescription> attachmentDescriptions = {};

        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
        ColorFormat generalDepthFormat = ColorFormat::R32F;
        m_ImageSamples = VulkanCommon::MsaaSampleCountToVulkan(createInfo.SampleCount);
        if (m_ImageSamples > device.MaxMsaaSamples())
            m_ImageSamples = device.MaxMsaaSamples();

        for (auto& attachment : createInfo.DepthAttachments)
        {
            VulkanFramebufferAttachment attachmentData = {};
            attachmentData.GeneralColorFormat = generalDepthFormat;
            attachmentData.ColorFormat = depthFormat;
            attachmentData.HasResolve = false; //m_ImageSamples > VK_SAMPLE_COUNT_1_BIT;
            attachmentData.CPUVisible = attachment.AllowCPURead;
            attachmentData.IsDepthAttachment = true;

            CreateAttachmentImages(attachmentData, depthFormat);

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = depthFormat;
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

            attachmentData.ImageAttachmentIndex = attachmentIndex++;

            m_DepthAttachmentData.emplace_back(attachmentData);
        }

        // TODO: depth resolve (https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/performance/msaa/msaa_tutorial.md)
        for (auto& attachment : createInfo.ColorAttachments)
        {
            VkFormat colorFormat = VulkanCommon::ColorFormatToVulkan(attachment.Format);
            VkClearValue clearValue{};
            clearValue.color = { attachment.ClearColor.r, attachment.ClearColor.g, attachment.ClearColor.b, attachment.ClearColor.a };

            VulkanFramebufferAttachment attachmentData = {};
            attachmentData.GeneralColorFormat = attachment.Format;
            attachmentData.ColorFormat = colorFormat;
            attachmentData.HasResolve = m_ImageSamples > VK_SAMPLE_COUNT_1_BIT;
            attachmentData.CPUVisible = attachment.AllowCPURead;
            attachmentData.IsDepthAttachment = false;

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

            attachmentData.ImageAttachmentIndex = attachmentIndex;
            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = attachmentIndex++;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentRefs.emplace_back(colorAttachmentRef);

            attachmentData.ResolveImageAttachmentIndex = attachmentIndex;
            VkAttachmentReference colorAttachmentResolveRef{};
            colorAttachmentResolveRef.attachment = attachmentData.HasResolve ? attachmentIndex++ : VK_ATTACHMENT_UNUSED;
            colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveAttachmentRefs.emplace_back(colorAttachmentResolveRef);

            m_AttachmentData.emplace_back(attachmentData);
        }

        std::vector<VkSubpassDescription> subpasses(createInfo.Subpasses.size());
        std::vector<VkSubpassDependency> dependencies(createInfo.Subpasses.size());
        std::vector<VkAttachmentReference> inputAttachmentRefs;
        std::vector<VkAttachmentReference> outputAttachmentRefs;
        std::vector<VkAttachmentReference> outputResolveAttachmentRefs;
        std::vector<VkAttachmentReference> depthAttachmentRefs;
        for (size_t i = 0; i < subpasses.size(); i++)
        {
            HE_ENGINE_ASSERT(i != 0 || inputAttachmentRefs.size() == 0, "Subpass 0 cannot have input attachments");

            depthAttachmentRefs.push_back({ VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });

            u32 inputAttachmentCount = 0;
            u32 outputAttachmentCount = 0;
            for (size_t j = 0; j < createInfo.Subpasses[i].InputAttachments.size(); j++)
            {
                auto& attachment = createInfo.Subpasses[i].InputAttachments[j];
                if (attachment.Type == SubpassAttachmentType::ColorAttachment)
                    inputAttachmentRefs.push_back({ m_AttachmentData[attachment.AttachmentIndex].ImageAttachmentIndex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
                else
                    inputAttachmentRefs.push_back({ m_DepthAttachmentData[attachment.AttachmentIndex].ImageAttachmentIndex, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL });
                inputAttachmentCount++;
            }
            for (size_t j = 0; j < createInfo.Subpasses[i].OutputAttachments.size(); j++)
            {
                auto& attachment = createInfo.Subpasses[i].OutputAttachments[j];
                if (attachment.Type == SubpassAttachmentType::DepthAttachment)
                {
                    HE_ENGINE_ASSERT(depthAttachmentRefs[i].attachment == VK_ATTACHMENT_UNUSED, "Cannot bind more than one depth attachment to the output of a subpass");
                    depthAttachmentRefs[i].attachment = m_DepthAttachmentData[attachment.AttachmentIndex].ImageAttachmentIndex;
                }
                else
                {
                    outputAttachmentRefs.push_back({ m_AttachmentData[attachment.AttachmentIndex].ImageAttachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
                    outputResolveAttachmentRefs.push_back({ m_AttachmentData[attachment.AttachmentIndex].HasResolve ? m_AttachmentData[attachment.AttachmentIndex].ResolveImageAttachmentIndex : VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
                    outputAttachmentCount++;
                }

            }

            subpasses[i] = {};
            subpasses[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpasses[i].colorAttachmentCount = outputAttachmentCount;
            subpasses[i].pColorAttachments = outputAttachmentRefs.data() + outputAttachmentRefs.size() - outputAttachmentCount;
            subpasses[i].pResolveAttachments = outputResolveAttachmentRefs.data();
            subpasses[i].pDepthStencilAttachment = depthAttachmentRefs.data() + i;
            subpasses[i].inputAttachmentCount = inputAttachmentCount;
            subpasses[i].pInputAttachments = inputAttachmentRefs.data() + inputAttachmentRefs.size() - inputAttachmentCount;

            dependencies[i] = {};
            dependencies[i].srcSubpass = static_cast<u32>(i - 1);
            dependencies[i].dstSubpass = static_cast<u32>(i);
            dependencies[i].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[i].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[i].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[i].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            if (i == 0)
            {
                dependencies[i].srcSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[i].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                dependencies[i].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                dependencies[i].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[i].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; //| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }
            if (i == subpasses.size() - 1)
            {
                // if (i == 0)
                //     dependencies[i].dstSubpass = 0;
                // else
                //     dependencies[i].dstSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[i].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; //| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependencies[i].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                dependencies[i].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            }
        }

        // subpasses[0] = {};
        // subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        // subpasses[0].colorAttachmentCount = static_cast<u32>(colorAttachmentRefs.size());
        // subpasses[0].pColorAttachments = colorAttachmentRefs.data();
        // subpasses[0].pResolveAttachments = resolveAttachmentRefs.data();
        // subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;

        // dependencies[0] = {};
        // dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        // dependencies[0].dstSubpass = 0;
        // dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        // dependencies[0].srcAccessMask = 0;
        // dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        // dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<u32>(attachmentDescriptions.size());
        renderPassInfo.pAttachments = attachmentDescriptions.data();
        renderPassInfo.subpassCount = static_cast<u32>(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();
        renderPassInfo.dependencyCount = static_cast<u32>(dependencies.size());;
        renderPassInfo.pDependencies = dependencies.data();
          
        HE_VULKAN_CHECK_RESULT(vkCreateRenderPass(device.Device(), &renderPassInfo, nullptr, &m_RenderPass));

        CreateFramebuffer();
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext::Sync();

        CleanupFramebuffer();

        for (auto& attachmentData : m_AttachmentData)
            CleanupAttachmentImages(attachmentData);
        for (auto& attachmentData : m_DepthAttachmentData)
            CleanupAttachmentImages(attachmentData);

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
        for (auto& attachment : m_AttachmentData)
            CopyAttachmentToBuffer(attachment);
        for (auto& attachment : m_DepthAttachmentData)
            CopyAttachmentToBuffer(attachment);

        HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(buffer));

        m_BoundPipeline = "";
        m_BoundThisFrame = false;
        m_SubmittedThisFrame = true;
    }

    void VulkanFramebuffer::CopyAttachmentToBuffer(VulkanFramebufferAttachment& attachmentData)
    {
        if (!attachmentData.CPUVisible)
            return;

        VkCommandBuffer buffer = GetCommandBuffer();

        VkBufferImageCopy copyData{};
        copyData.bufferOffset = 0;
        copyData.imageSubresource.baseArrayLayer = 0;
        copyData.imageSubresource.layerCount = 1;
        copyData.imageSubresource.mipLevel = 0;
        copyData.imageSubresource.aspectMask = attachmentData.IsDepthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        copyData.imageExtent = { m_ActualWidth, m_ActualHeight, 1 };

        VkImage image = attachmentData.Image;
        if (attachmentData.HasResolve)
            image = attachmentData.ResolveImage;

        // copy it to the device local staging buffer
        //VulkanCommon::TransitionImageLayout(VulkanContext::GetDevice().Device(), buffer, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkImageMemoryBarrier imageBarrier = {};

        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.subresourceRange.aspectMask = attachmentData.IsDepthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange.levelCount = 1;
        imageBarrier.subresourceRange.layerCount = 1;

        imageBarrier.oldLayout       = attachmentData.IsDepthAttachment ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier.newLayout       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageBarrier.srcAccessMask   = 0;
        imageBarrier.dstAccessMask   = VK_ACCESS_TRANSFER_READ_BIT;
        imageBarrier.image           = image;

        VkBufferMemoryBarrier bufferBarrier = {};

        bufferBarrier.sType           = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferBarrier.srcAccessMask   = 0;
        bufferBarrier.dstAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
        bufferBarrier.buffer          = attachmentData.AttachmentBuffer->GetBuffer();
        bufferBarrier.size            = attachmentData.AttachmentBuffer->GetAllocatedSize();

        vkCmdPipelineBarrier(buffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            1,
            &bufferBarrier,
            1,
            &imageBarrier);
        vkCmdCopyImageToBuffer(buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, attachmentData.AttachmentBuffer->GetBuffer(), 1, &copyData);
        //VulkanCommon::TransitionImageLayout(VulkanContext::GetDevice().Device(), buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
        HE_ENGINE_ASSERT(attachmentIndex < m_AttachmentData.size(), "Color attachment access on framebuffer out of range");
 
        return (m_AttachmentData[attachmentIndex].HasResolve ? m_AttachmentData[attachmentIndex].ResolveImageImGuiId : m_AttachmentData[attachmentIndex].ImageImGuiId);
    }

    void* VulkanFramebuffer::GetDepthAttachmentImGuiHandle(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(m_Info.SampleCount == MsaaSampleCount::None, "Cannot get framebuffer depth attachment handle, SampleCount != None");
        HE_ENGINE_ASSERT(attachmentIndex < m_DepthAttachmentData.size(), "Depth attachment access on framebuffer out of range");
 
        return (m_DepthAttachmentData[attachmentIndex].HasResolve ? m_DepthAttachmentData[attachmentIndex].ResolveImageImGuiId : m_DepthAttachmentData[attachmentIndex].ImageImGuiId);
    }

    void* VulkanFramebuffer::GetColorAttachmentPixelData(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_AttachmentData.size(), "Color attachment access on framebuffer out of range");
        HE_ENGINE_ASSERT(m_AttachmentData[attachmentIndex].CPUVisible, "Cannot read pixel data of color attachment that does not have 'AllowCPURead' enabled");

        return m_AttachmentData[attachmentIndex].AttachmentBuffer->GetMappedMemory();
    }

    void* VulkanFramebuffer::GetDepthAttachmentPixelData(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_DepthAttachmentData.size(), "Depth attachment access on framebuffer out of range");
        HE_ENGINE_ASSERT(m_DepthAttachmentData[attachmentIndex].CPUVisible, "Cannot read pixel data of depth attachment that does not have 'AllowCPURead' enabled");

        return m_DepthAttachmentData[attachmentIndex].AttachmentBuffer->GetMappedMemory();
    }

    void VulkanFramebuffer::StartNextSubpass()
    {
        vkCmdNextSubpass(GetCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);
    }

    Ref<GraphicsPipeline> VulkanFramebuffer::InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.BlendStates.size() == m_Info.ColorAttachments.size(), "Graphics pipeline blend state count must match framebuffer color attachment count");

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
            (attachmentData.IsDepthAttachment ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) | VK_IMAGE_USAGE_SAMPLED_BIT | (colorTransferSrc ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            attachmentData.Image,
            attachmentData.ImageMemory
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
                (attachmentData.IsDepthAttachment ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) | VK_IMAGE_USAGE_SAMPLED_BIT | (attachmentData.CPUVisible ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                attachmentData.ResolveImage,
                attachmentData.ResolveImageMemory
            );
        }

        if (attachmentData.CPUVisible)
        {
            attachmentData.AttachmentBuffer = std::dynamic_pointer_cast<VulkanBuffer>(Buffer::Create(
                Buffer::Type::Pixel,
                BufferUsageType::Dynamic,
                { ColorFormatBufferDataType(attachmentData.GeneralColorFormat) },
                m_ActualWidth * m_ActualHeight * ColorFormatComponents(attachmentData.GeneralColorFormat)
            ));
        }

        // create associated image views 
        attachmentData.ImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.Image, colorFormat, 1, attachmentData.IsDepthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
        m_CachedImageViews.emplace_back(attachmentData.ImageView);
        attachmentData.ImageImGuiId = ImGui_ImplVulkan_AddTexture(VulkanContext::GetDefaultSampler(), attachmentData.ImageView, attachmentData.IsDepthAttachment ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        if (attachmentData.HasResolve)
        {
            attachmentData.ResolveImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.ResolveImage, colorFormat, 1, attachmentData.IsDepthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
            m_CachedImageViews.emplace_back(attachmentData.ResolveImageView);
            attachmentData.ResolveImageImGuiId = ImGui_ImplVulkan_AddTexture(VulkanContext::GetDefaultSampler(), attachmentData.ResolveImageView, attachmentData.IsDepthAttachment ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    void VulkanFramebuffer::CleanupAttachmentImages(VulkanFramebufferAttachment& attachmentData)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        ImGui_ImplVulkan_RemoveTexture(attachmentData.ImageImGuiId);
            
        vkDestroyImageView(device.Device(), attachmentData.ImageView, nullptr);
        vkDestroyImage(device.Device(), attachmentData.Image, nullptr);
        vkFreeMemory(device.Device(), attachmentData.ImageMemory, nullptr);

        if (attachmentData.HasResolve)
        {
            ImGui_ImplVulkan_RemoveTexture(attachmentData.ResolveImageImGuiId);

            vkDestroyImageView(device.Device(), attachmentData.ResolveImageView, nullptr);
            vkDestroyImage(device.Device(), attachmentData.ResolveImage, nullptr);
            vkFreeMemory(device.Device(), attachmentData.ResolveImageMemory, nullptr);
        }

        attachmentData.AttachmentBuffer.reset();
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
            CreateAttachmentImages(attachmentData, attachmentData.ColorFormat);
        }

        for (auto& attachmentData : m_DepthAttachmentData)
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
