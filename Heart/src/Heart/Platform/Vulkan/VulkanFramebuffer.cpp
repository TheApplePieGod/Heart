#include "hepch.h"
#include "VulkanFramebuffer.h"

#include "Heart/Core/Window.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "Heart/Platform/Vulkan/VulkanTexture.h"
#include "Heart/Platform/Vulkan/VulkanBuffer.h"
#include "Heart/Platform/Vulkan/VulkanGraphicsPipeline.h"
#include "Heart/Platform/Vulkan/VulkanComputePipeline.h"
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
            attachmentData.CPUVisible = false;
            attachmentData.IsDepthAttachment = true;
            attachmentData.ExternalTexture = nullptr;

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

            for (u32 frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
            {
                CreateAttachmentImages(attachmentData, frame);
                m_DepthAttachmentData[frame].emplace_back(attachmentData);
            }
        }

        // TODO: depth resolve (https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/performance/msaa/msaa_tutorial.md)
        for (auto& attachment : createInfo.ColorAttachments)
        {
            VulkanFramebufferAttachment attachmentData = {};
            if (attachment.Texture)
            {
                attachmentData.ExternalTexture = (VulkanTexture*)attachment.Texture.get();
                attachmentData.ExternalTextureLayer = attachment.LayerIndex;
                attachmentData.ExternalTextureMip = attachment.MipLevel;

                HE_ENGINE_ASSERT(m_Info.Width == attachmentData.ExternalTexture->GetMipWidth(attachment.MipLevel), "Texture dimensions (at the specified mip level) must match the framebuffer (framebuffer width/height cannot be zero)");
                HE_ENGINE_ASSERT(m_Info.Height == attachmentData.ExternalTexture->GetMipHeight(attachment.MipLevel), "Texture dimensions (at the specified mip level) must match the framebuffer (framebuffer width/height cannot be zero)");
            }

            VkFormat colorFormat = attachment.Texture ? attachmentData.ExternalTexture->GetFormat() : VulkanCommon::ColorFormatToVulkan(attachment.Format);
            VkClearValue clearValue{};
            clearValue.color = { attachment.ClearColor.r, attachment.ClearColor.g, attachment.ClearColor.b, attachment.ClearColor.a };

            attachmentData.GeneralColorFormat = attachment.Texture ? attachmentData.ExternalTexture->GetGeneralFormat() : attachment.Format;
            attachmentData.ColorFormat = colorFormat;
            attachmentData.HasResolve = m_ImageSamples > VK_SAMPLE_COUNT_1_BIT;
            attachmentData.CPUVisible = attachmentData.ExternalTexture ? attachmentData.ExternalTexture->CanCPURead() : attachment.AllowCPURead;
            attachmentData.IsDepthAttachment = false;

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

            for (u32 frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
            {
                CreateAttachmentImages(attachmentData, frame);
                m_AttachmentData[frame].emplace_back(attachmentData);
            }
        }

        std::vector<VkSubpassDescription> subpasses(createInfo.Subpasses.size());
        std::vector<VkSubpassDependency> dependencies(createInfo.Subpasses.size());
        std::vector<VkAttachmentReference> inputAttachmentRefs;
        std::vector<VkAttachmentReference> outputAttachmentRefs;
        std::vector<VkAttachmentReference> outputResolveAttachmentRefs;
        std::vector<VkAttachmentReference> depthAttachmentRefs;

        inputAttachmentRefs.reserve(100);
        outputAttachmentRefs.reserve(100);
        outputResolveAttachmentRefs.reserve(100);
        depthAttachmentRefs.reserve(createInfo.Subpasses.size());
        for (size_t i = 0; i < subpasses.size(); i++)
        {
            HE_ENGINE_ASSERT(i != 0 || inputAttachmentRefs.size() == 0, "Subpass 0 cannot have input attachments");

            depthAttachmentRefs.push_back({ VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL });

            u32 inputAttachmentCount = 0;
            u32 outputAttachmentCount = 0;
            for (size_t j = 0; j < createInfo.Subpasses[i].InputAttachments.size(); j++)
            {
                auto& attachment = createInfo.Subpasses[i].InputAttachments[j];
                if (attachment.Type == SubpassAttachmentType::Color)
                    inputAttachmentRefs.push_back({ m_AttachmentData[0][attachment.AttachmentIndex].HasResolve ? m_AttachmentData[0][attachment.AttachmentIndex].ResolveImageAttachmentIndex : m_AttachmentData[0][attachment.AttachmentIndex].ImageAttachmentIndex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
                else
                    inputAttachmentRefs.push_back({ m_DepthAttachmentData[0][attachment.AttachmentIndex].ImageAttachmentIndex, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL });
                inputAttachmentCount++;
            }
            for (size_t j = 0; j < createInfo.Subpasses[i].OutputAttachments.size(); j++)
            {
                auto& attachment = createInfo.Subpasses[i].OutputAttachments[j];
                if (attachment.Type == SubpassAttachmentType::Depth)
                {
                    HE_ENGINE_ASSERT(depthAttachmentRefs[i].attachment == VK_ATTACHMENT_UNUSED, "Cannot bind more than one depth attachment to the output of a subpass");
                    depthAttachmentRefs[i].attachment = m_DepthAttachmentData[0][attachment.AttachmentIndex].ImageAttachmentIndex;
                }
                else
                {
                    outputAttachmentRefs.push_back({ m_AttachmentData[0][attachment.AttachmentIndex].ImageAttachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
                    outputResolveAttachmentRefs.push_back({ m_AttachmentData[0][attachment.AttachmentIndex].HasResolve ? m_AttachmentData[0][attachment.AttachmentIndex].ResolveImageAttachmentIndex : VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
                    outputAttachmentCount++;
                }

            }

            subpasses[i] = {};
            subpasses[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpasses[i].colorAttachmentCount = outputAttachmentCount;
            subpasses[i].pColorAttachments = outputAttachmentRefs.data() + outputAttachmentRefs.size() - outputAttachmentCount;
            subpasses[i].pResolveAttachments = outputResolveAttachmentRefs.data() + outputResolveAttachmentRefs.size() - outputAttachmentCount;
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
                dependencies[i].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; //| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependencies[i].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                dependencies[i].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            }
        }

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

        for (u32 frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
        {
            for (auto& attachmentData : m_AttachmentData[frame])
                CleanupAttachmentImages(attachmentData);
            for (auto& attachmentData : m_DepthAttachmentData[frame])
                CleanupAttachmentImages(attachmentData);
        }

        vkDestroyRenderPass(device.Device(), m_RenderPass, nullptr);

        FreeCommandBuffers();
    }

    void VulkanFramebuffer::Bind(ComputePipeline* preRenderComputePipeline)
    {
        HE_PROFILE_FUNCTION();

        UpdateFrameIndex();

        HE_ENGINE_ASSERT(!m_SubmittedThisFrame, "Cannot bind a framebuffer that has already been submitted");

        VkCommandBuffer buffer = GetCommandBuffer();
        VulkanContext::SetBoundFramebuffer(this);

        if (!m_BoundThisFrame)
        {
            m_BoundThisFrame = true;
            if (!m_Valid)
                Recreate();
     
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            HE_VULKAN_CHECK_RESULT(vkBeginCommandBuffer(buffer, &beginInfo));

            if (preRenderComputePipeline)
            {
                VulkanComputePipeline* comp = (VulkanComputePipeline*)preRenderComputePipeline;
                vkCmdPipelineBarrier(
                    GetCommandBuffer(),
                    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    0, 0, nullptr, 0, nullptr, 0, nullptr
                );

                comp->Submit();
                VkCommandBuffer pipelineBuf = comp->GetInlineCommandBuffer();
                vkCmdExecuteCommands(VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(), 1, &pipelineBuf);

                vkCmdPipelineBarrier(
                    VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(),
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                    0, 0, nullptr, 0, nullptr, 0, nullptr
                );
            }

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

            vkCmdSetLineWidth(buffer, 1.f);

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = m_RenderPass;
            renderPassInfo.framebuffer = m_Framebuffers[m_InFlightFrameIndex];
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = { m_ActualWidth, m_ActualHeight };

            renderPassInfo.clearValueCount = static_cast<u32>(m_CachedClearValues.size());
            renderPassInfo.pClearValues = m_CachedClearValues.data();

            vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        }
    }

    void VulkanFramebuffer::Submit(ComputePipeline* postRenderComputePipeline)
    {
        HE_PROFILE_FUNCTION();

        HE_ENGINE_ASSERT(!m_SubmittedThisFrame, "Cannot submit framebuffer twice in the same frame");
        HE_ENGINE_ASSERT(m_SubmittedThisFrame || m_BoundThisFrame, "Cannot submit framebuffer that has not been bound this frame");
        HE_ENGINE_ASSERT(m_CurrentSubpass == m_Info.Subpasses.size() - 1, "Attempting to submit a framebuffer without completing all subpasses");

        if (!m_SubmittedThisFrame)
        {
            VulkanDevice& device = VulkanContext::GetDevice();
            VkCommandBuffer buffer = GetCommandBuffer();
            VkCommandBuffer transferBuffer = GetTransferCommandBuffer();

            vkCmdEndRenderPass(buffer);

            // run the post render compute if applicable
            if (postRenderComputePipeline)
            {
                VulkanComputePipeline* comp = (VulkanComputePipeline*)postRenderComputePipeline;
                vkCmdPipelineBarrier(
                    GetCommandBuffer(),
                    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    0, 0, nullptr, 0, nullptr, 0, nullptr
                );

                comp->Submit();
                VkCommandBuffer pipelineBuf = comp->GetInlineCommandBuffer();
                vkCmdExecuteCommands(VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(), 1, &pipelineBuf);
            }

            // if the transfer buffer is null, then we have no CPU visible attachments, so don't bother running this code
            if (transferBuffer)
            {
                // begin the transfer command buffer
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                beginInfo.pInheritanceInfo = nullptr;

                HE_VULKAN_CHECK_RESULT(vkBeginCommandBuffer(transferBuffer, &beginInfo));

                // copy all CPU visible attachments to their respective buffers        
                for (auto& attachment : m_AttachmentData[m_InFlightFrameIndex])
                {
                    if (!attachment.CPUVisible) continue;
                    CopyAttachmentToBuffer(attachment);
                    VulkanCommon::CopyBufferToBuffer(
                        transferBuffer,
                        attachment.AttachmentBuffer->GetStagingBuffer(),
                        attachment.AttachmentBuffer->GetBuffer(),
                        attachment.AttachmentBuffer->GetAllocatedSize()
                    );
                }
                for (auto& attachment : m_DepthAttachmentData[m_InFlightFrameIndex])
                {
                    if (!attachment.CPUVisible) continue;
                    CopyAttachmentToBuffer(attachment);
                    VulkanCommon::CopyBufferToBuffer(
                        transferBuffer,
                        attachment.AttachmentBuffer->GetStagingBuffer(),
                        attachment.AttachmentBuffer->GetBuffer(),
                        attachment.AttachmentBuffer->GetAllocatedSize()
                    );
                }

                // end the transfer command buffer
                HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(transferBuffer));
            }

            // execute any commands that need to be synced with this framebuffer (i.e. Texture::RegenerateMipMapsSync)
            for (auto cmdBuf : m_AuxiliaryCommandBuffers[m_InFlightFrameIndex])
                vkCmdExecuteCommands(buffer, 1, &cmdBuf);

            // end the main command buffer
            HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(buffer));

            m_CurrentPipelineStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
            m_BoundPipeline = nullptr;
            m_BoundPipelineName = "";
            m_BoundThisFrame = false;
            m_SubmittedThisFrame = true;
            m_FlushedThisFrame = false;
            m_CurrentSubpass = 0;
        }
    }

    void VulkanFramebuffer::CopyAttachmentToBuffer(VulkanFramebufferAttachment& attachmentData)
    {
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
        bufferBarrier.buffer          = attachmentData.AttachmentBuffer->GetStagingBuffer();
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
        vkCmdCopyImageToBuffer(buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, attachmentData.AttachmentBuffer->GetStagingBuffer(), 1, &copyData);
        //VulkanCommon::TransitionImageLayout(VulkanContext::GetDevice().Device(), buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void VulkanFramebuffer::BindPipeline(const std::string& name)
    {
        HE_PROFILE_FUNCTION();

        if (name == m_BoundPipelineName) return;

        VkCommandBuffer buffer = GetCommandBuffer();
        auto pipeline = static_cast<VulkanGraphicsPipeline*>(LoadPipeline(name).get());

        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

        m_BoundPipeline = pipeline;
        m_BoundPipelineName = name;
    }

    void VulkanFramebuffer::BindShaderBufferResource(u32 bindingIndex, u32 elementOffset, u32 elementCount, Buffer* buffer)
    {
        HE_PROFILE_FUNCTION();

        HE_ENGINE_ASSERT(elementCount + elementOffset <= buffer->GetAllocatedCount(), "ElementCount + ElementOffset must be <= buffer allocated count");
        HE_ENGINE_ASSERT(buffer->GetType() == Buffer::Type::Uniform || buffer->GetType() == Buffer::Type::Storage, "Buffer bind must be either a uniform or storage buffer");

        ShaderResourceType bufferType = buffer->GetType() == Buffer::Type::Uniform ? ShaderResourceType::UniformBuffer : ShaderResourceType::StorageBuffer;
        BindShaderResource(bindingIndex, bufferType, buffer, true, buffer->GetLayout().GetStride() * elementOffset, buffer->GetLayout().GetStride() * elementCount);
    }

    void VulkanFramebuffer::BindShaderTextureResource(u32 bindingIndex, Texture* texture)
    {
        HE_PROFILE_FUNCTION();
        
        HE_ENGINE_ASSERT(
            std::find_if(
                m_Info.ColorAttachments.begin(),
                m_Info.ColorAttachments.end(),
                [texture](FramebufferColorAttachment& att){ return att.Texture.get() == texture; }
            ) == m_Info.ColorAttachments.end(),
            "Cannot bind a texture resource that is currently being written to (use BindShaderTextureLayerResource)"
        );

        BindShaderResource(bindingIndex, ShaderResourceType::Texture, texture, false, 0, 0);
    }

    void VulkanFramebuffer::BindShaderTextureLayerResource(u32 bindingIndex, Texture* texture, u32 layerIndex, u32 mipLevel)
    {
        HE_PROFILE_FUNCTION();

        HE_ENGINE_ASSERT(
            std::find_if(
                m_Info.ColorAttachments.begin(),
                m_Info.ColorAttachments.end(),
                [texture, layerIndex, mipLevel](FramebufferColorAttachment& att){ return att.Texture.get() == texture && att.LayerIndex == layerIndex && att.MipLevel == mipLevel; }
            ) == m_Info.ColorAttachments.end(),
            "Cannot bind a texture resource that is currently being written to (mip level & layer must not be a current color attachment)"
        );

        BindShaderResource(bindingIndex, ShaderResourceType::Texture, texture, true, layerIndex, mipLevel);
    }

    void VulkanFramebuffer::BindSubpassInputAttachment(u32 bindingIndex, SubpassAttachment attachment)
    {
        HE_PROFILE_FUNCTION();

        BindShaderResource(bindingIndex, ShaderResourceType::SubpassInput, attachment.Type == SubpassAttachmentType::Depth ? &m_DepthAttachmentData[m_InFlightFrameIndex][attachment.AttachmentIndex] : &m_AttachmentData[m_InFlightFrameIndex][attachment.AttachmentIndex], false, 0, 0);
    }

    void VulkanFramebuffer::BindShaderResource(u32 bindingIndex, ShaderResourceType resourceType, void* resource, bool useOffset, u32 offset, u32 size)
    {
        HE_ENGINE_ASSERT(m_BoundPipeline != nullptr, "Must call BindPipeline before BindShaderResource");
        HE_ENGINE_ASSERT(resource != nullptr, "Cannot bind a null resource to a shader");

        VulkanDescriptorSet& descriptorSet = m_BoundPipeline->GetVulkanDescriptorSet();

        if (!descriptorSet.DoesBindingExist(bindingIndex))
            return; // silently ignore, TODO: warning once in the console when this happens

        HE_ENGINE_ASSERT(descriptorSet.IsResourceCorrectType(bindingIndex, resourceType), "Attempting to bind a resource that does not match the bind index type");

        if (useOffset && (resourceType == ShaderResourceType::UniformBuffer || resourceType == ShaderResourceType::StorageBuffer))
            descriptorSet.UpdateDynamicOffset(bindingIndex, offset);

        // update the descriptor set binding information
        descriptorSet.UpdateShaderResource(bindingIndex, resourceType, resource, useOffset, offset, size);
    }

    void VulkanFramebuffer::FlushBindings()
    {
        HE_ENGINE_ASSERT(m_BoundPipeline != nullptr, "Must call BindPipeline and bind all resources before FlushBindings");

        VulkanDescriptorSet& descriptorSet = m_BoundPipeline->GetVulkanDescriptorSet();

        // update a newly allocated descriptor set based on the current bindings or return
        // a cached one that was created before with the same binding info
        descriptorSet.FlushBindings();

        HE_ENGINE_ASSERT(descriptorSet.GetMostRecentDescriptorSet() != nullptr);

        // bind the new set
        VkDescriptorSet sets[1] = { descriptorSet.GetMostRecentDescriptorSet() };
        vkCmdBindDescriptorSets(
            GetCommandBuffer(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_BoundPipeline->GetLayout(),
            0, 1,
            sets,
            static_cast<u32>(descriptorSet.GetDynamicOffsets().size()),
            descriptorSet.GetDynamicOffsets().data()
        );

        m_FlushedThisFrame = true;
    }
    
    void* VulkanFramebuffer::GetColorAttachmentImGuiHandle(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_AttachmentData[m_InFlightFrameIndex].size(), "Color attachment access on framebuffer out of range");
 
        return (
            m_AttachmentData[m_InFlightFrameIndex][attachmentIndex].HasResolve ?
            m_AttachmentData[m_InFlightFrameIndex][attachmentIndex].ResolveImageImGuiId :
            m_AttachmentData[m_InFlightFrameIndex][attachmentIndex].ImageImGuiId
        );
    }

    void* VulkanFramebuffer::GetColorAttachmentPixelData(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_AttachmentData[m_InFlightFrameIndex].size(), "Color attachment access on framebuffer out of range");
        HE_ENGINE_ASSERT(m_AttachmentData[m_InFlightFrameIndex][attachmentIndex].CPUVisible, "Cannot read pixel data of color attachment that does not have 'AllowCPURead' enabled");

        return m_AttachmentData[m_InFlightFrameIndex][attachmentIndex].AttachmentBuffer->GetMappedMemory();
    }

    void VulkanFramebuffer::StartNextSubpass()
    {
        m_CurrentSubpass++;
        HE_ENGINE_ASSERT(m_CurrentSubpass < m_Info.Subpasses.size(), "Attempting to start a subpass that does not exist");

        vkCmdNextSubpass(GetCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);

        m_FlushedThisFrame = false;
        m_BoundPipeline = nullptr;
        m_BoundPipelineName = "";
    }

    void VulkanFramebuffer::ClearOutputAttachment(u32 outputAttachmentIndex, bool clearDepth)
    {
        VkClearAttachment clear[2];
        clear[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clear[0].clearValue.color.float32[0] = 0.0f;
        clear[0].clearValue.color.float32[1] = 0.0f;
        clear[0].clearValue.color.float32[2] = 0.0f;
        clear[0].clearValue.color.float32[3] = 0.0f;
        clear[0].colorAttachment = outputAttachmentIndex;
        clear[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        clear[1].clearValue.depthStencil.depth = Renderer::IsUsingReverseDepth() ? 0.f : 1.0f;
        clear[1].clearValue.depthStencil.stencil = 0;
        VkClearRect clearRect;
        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = 1;
        clearRect.rect.extent.height = m_ActualHeight;
        clearRect.rect.extent.width = m_ActualWidth;
        clearRect.rect.offset.x = 0;
        clearRect.rect.offset.y = 0;
        vkCmdClearAttachments(GetCommandBuffer(), clearDepth ? 2 : 1, clear, 1, &clearRect);
    }

    Ref<GraphicsPipeline> VulkanFramebuffer::InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.SubpassIndex >= 0 && createInfo.SubpassIndex < m_Info.Subpasses.size(), "Graphics pipeline subpass index is out of range");
        HE_ENGINE_ASSERT(createInfo.BlendStates.size() == GetSubpassOutputColorAttachmentCount(createInfo.SubpassIndex), "Graphics pipeline blend state count must match subpass color attachment output count");

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

        // only allocate the transfer command buffers if we have any cpu visible attachments
        for (auto& attachment : m_Info.ColorAttachments)
        {
            if (attachment.AllowCPURead)
            {
                allocInfo.commandPool = VulkanContext::GetTransferPool();
                allocInfo.commandBufferCount = static_cast<u32>(m_TransferCommandBuffers.size());
                HE_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device.Device(), &allocInfo, m_TransferCommandBuffers.data()));
                break;
            }
        } 
    }

    void VulkanFramebuffer::FreeCommandBuffers()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkFreeCommandBuffers(device.Device(), VulkanContext::GetGraphicsPool(), static_cast<u32>(m_CommandBuffers.size()), m_CommandBuffers.data());
        if (GetTransferCommandBuffer())
            vkFreeCommandBuffers(device.Device(), VulkanContext::GetTransferPool(), static_cast<u32>(m_TransferCommandBuffers.size()), m_TransferCommandBuffers.data());
    }

    void VulkanFramebuffer::CreateAttachmentImages(VulkanFramebufferAttachment& attachmentData, u32 inFlightFrameIndex)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        bool colorTransferSrc = attachmentData.CPUVisible && !attachmentData.HasResolve;

        // create the associated images for the framebuffer
        // if we are using an external texture, that texture will be the resolve image in the case of a multisampled framebuffer
        // for one sample, we will render directly to that image

        bool shouldResolveExternal = attachmentData.ExternalTexture && attachmentData.HasResolve;
        bool shouldCreateBaseTexture = !attachmentData.ExternalTexture || shouldResolveExternal;
        bool shouldCreateResolveTexture = attachmentData.HasResolve && !attachmentData.ExternalTexture;
        if (shouldCreateBaseTexture)
        {
            VulkanCommon::CreateImage(
                device.Device(),
                device.PhysicalDevice(),
                m_ActualWidth,
                m_ActualHeight,
                attachmentData.ColorFormat,
                1, 1,
                m_ImageSamples,
                VK_IMAGE_TILING_OPTIMAL,
                (attachmentData.IsDepthAttachment ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | (colorTransferSrc ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                attachmentData.Image,
                attachmentData.ImageMemory,
                VK_IMAGE_LAYOUT_UNDEFINED
            );
        }

        if (shouldCreateResolveTexture)
        {
            VulkanCommon::CreateImage(
                device.Device(),
                device.PhysicalDevice(),
                m_ActualWidth,
                m_ActualHeight,
                attachmentData.ColorFormat,
                1, 1,
                VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_TILING_OPTIMAL,
                (attachmentData.IsDepthAttachment ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | (attachmentData.CPUVisible ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                attachmentData.ResolveImage,
                attachmentData.ResolveImageMemory,
                VK_IMAGE_LAYOUT_UNDEFINED
            );
        }

        if (attachmentData.CPUVisible)
        {
            if (attachmentData.ExternalTexture)
                attachmentData.AttachmentBuffer = attachmentData.ExternalTexture->GetCpuBuffer();
            else
            {
                attachmentData.AttachmentBuffer = std::dynamic_pointer_cast<VulkanBuffer>(Buffer::Create(
                    Buffer::Type::Pixel,
                    BufferUsageType::Dynamic,
                    { ColorFormatBufferDataType(attachmentData.GeneralColorFormat) },
                    m_ActualWidth * m_ActualHeight * ColorFormatComponents(attachmentData.GeneralColorFormat)
                ));
            }
        }

        // create associated image views
        if (shouldCreateBaseTexture)
        {
            attachmentData.ImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.Image, attachmentData.ColorFormat, 1, 0, 1, 0, attachmentData.IsDepthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
            m_CachedImageViews[inFlightFrameIndex].emplace_back(attachmentData.ImageView);
            attachmentData.ImageImGuiId = ImGui_ImplVulkan_AddTexture(VulkanContext::GetDefaultSampler(), attachmentData.ImageView, attachmentData.IsDepthAttachment ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        if (shouldCreateResolveTexture)
        {
            attachmentData.ResolveImageView = VulkanCommon::CreateImageView(device.Device(), attachmentData.ResolveImage, attachmentData.ColorFormat, 1, 0, 1, 0, attachmentData.IsDepthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
            m_CachedImageViews[inFlightFrameIndex].emplace_back(attachmentData.ResolveImageView);
            attachmentData.ResolveImageImGuiId = ImGui_ImplVulkan_AddTexture(VulkanContext::GetDefaultSampler(), attachmentData.ResolveImageView, attachmentData.IsDepthAttachment ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        if (shouldResolveExternal)
        {
            attachmentData.ResolveImage = attachmentData.ExternalTexture->GetImage(inFlightFrameIndex);
            attachmentData.ResolveImageView = attachmentData.ExternalTexture->GetLayerImageView(inFlightFrameIndex, attachmentData.ExternalTextureLayer, attachmentData.ExternalTextureMip);
            attachmentData.ResolveImageMemory = attachmentData.ExternalTexture->GetImageMemory(inFlightFrameIndex);
            attachmentData.ResolveImageImGuiId = attachmentData.ExternalTexture->GetImGuiHandle(attachmentData.ExternalTextureLayer);
            m_CachedImageViews[inFlightFrameIndex].emplace_back(attachmentData.ResolveImageView);
        }
        else if (attachmentData.ExternalTexture)
        {
            attachmentData.Image = attachmentData.ExternalTexture->GetImage(inFlightFrameIndex);
            attachmentData.ImageView = attachmentData.ExternalTexture->GetLayerImageView(inFlightFrameIndex, attachmentData.ExternalTextureLayer, attachmentData.ExternalTextureMip);
            attachmentData.ImageMemory = attachmentData.ExternalTexture->GetImageMemory(inFlightFrameIndex);
            attachmentData.ImageImGuiId = attachmentData.ExternalTexture->GetImGuiHandle(attachmentData.ExternalTextureLayer);
            m_CachedImageViews[inFlightFrameIndex].emplace_back(attachmentData.ImageView);
        }
    }

    void VulkanFramebuffer::CleanupAttachmentImages(VulkanFramebufferAttachment& attachmentData)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        bool shouldResolveExternal = attachmentData.ExternalTexture && attachmentData.HasResolve;
        bool hasBaseTexture = !attachmentData.ExternalTexture || shouldResolveExternal;
        bool hasResolveTexture = attachmentData.HasResolve && !attachmentData.ExternalTexture;
        if (hasBaseTexture)
        {
            ImGui_ImplVulkan_RemoveTexture(attachmentData.ImageImGuiId);
            
            vkDestroyImageView(device.Device(), attachmentData.ImageView, nullptr);
            vkDestroyImage(device.Device(), attachmentData.Image, nullptr);
            vkFreeMemory(device.Device(), attachmentData.ImageMemory, nullptr);
        }

        if (hasResolveTexture)
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

        for (u32 frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
        {
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = static_cast<u32>(m_CachedImageViews[frame].size());
            framebufferInfo.pAttachments = m_CachedImageViews[frame].data();
            framebufferInfo.width = m_ActualWidth;
            framebufferInfo.height = m_ActualHeight;
            framebufferInfo.layers = 1;
            
            HE_VULKAN_CHECK_RESULT(vkCreateFramebuffer(device.Device(), &framebufferInfo, nullptr, &m_Framebuffers[frame]));
        }
    }

    void VulkanFramebuffer::CleanupFramebuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        
        for (u32 frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
            vkDestroyFramebuffer(device.Device(), m_Framebuffers[frame], nullptr);
    }

    void VulkanFramebuffer::Recreate()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext& mainContext = static_cast<VulkanContext&>(Window::GetMainWindow().GetContext());
        VulkanContext::Sync();

        CleanupFramebuffer();

        for (u32 frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
        {
            m_CachedImageViews[frame].clear();
            for (auto& attachmentData : m_DepthAttachmentData[frame])
            {
                CleanupAttachmentImages(attachmentData);
                CreateAttachmentImages(attachmentData, frame);
            }
            for (auto& attachmentData : m_AttachmentData[frame])
            {
                CleanupAttachmentImages(attachmentData);
                CreateAttachmentImages(attachmentData, frame);
            }
        }

        CreateFramebuffer();

        m_Valid = true;
    }

    void VulkanFramebuffer::UpdateFrameIndex()
    {
        if (App::Get().GetFrameCount() != m_LastUpdateFrame)
        {
            // it is a new frame so we need to get the new flight frame index
            VulkanDevice& device = VulkanContext::GetDevice();
            VulkanContext& mainContext = static_cast<VulkanContext&>(Window::GetMainWindow().GetContext());

            m_InFlightFrameIndex = mainContext.GetSwapChain().GetInFlightFrameIndex();
            m_LastUpdateFrame = App::Get().GetFrameCount();

            m_SubmittedThisFrame = false;

            // free old auxiliary command buffers
            if (m_AuxiliaryCommandBuffers[m_InFlightFrameIndex].size() > 0)
            {
                vkFreeCommandBuffers(device.Device(), VulkanContext::GetGraphicsPool(), static_cast<u32>(m_AuxiliaryCommandBuffers[m_InFlightFrameIndex].size()), m_AuxiliaryCommandBuffers[m_InFlightFrameIndex].data());
                m_AuxiliaryCommandBuffers[m_InFlightFrameIndex].clear();
            }
        }
    }
}
