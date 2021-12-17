#include "hepch.h"
#include "VulkanComputePipeline.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"
#include "Heart/Platform/Vulkan/VulkanShader.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanTexture.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Renderer/Renderer.h"

namespace Heart
{
    VulkanComputePipeline::VulkanComputePipeline(const ComputePipelineCreateInfo& createInfo)
        : ComputePipeline(createInfo)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        m_DescriptorSet.Initialize(m_ProgramReflectionData);

        auto computeShader = AssetManager::RetrieveAsset<ShaderAsset>(createInfo.ComputeShaderAsset)->GetShader();
        VkPipelineShaderStageCreateInfo shaderStage = VulkanCommon::DefineShaderStage(static_cast<VulkanShader*>(computeShader)->GetShaderModule(), VK_SHADER_STAGE_COMPUTE_BIT);

        std::vector<VkDescriptorSetLayout> layouts;
        layouts.emplace_back(m_DescriptorSet.GetLayout());

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<u32>(layouts.size());
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        HE_VULKAN_CHECK_RESULT(vkCreatePipelineLayout(device.Device(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = m_PipelineLayout;
        pipelineInfo.stage = shaderStage;
        HE_VULKAN_CHECK_RESULT(vkCreateComputePipelines(device.Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VulkanContext::GetComputePool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<u32>(m_CommandBuffers.size());

        HE_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device.Device(), &allocInfo, m_CommandBuffers.data()));

    }

    VulkanComputePipeline::~VulkanComputePipeline()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext::Sync();

        vkDestroyPipeline(device.Device(), m_Pipeline, nullptr);
        vkDestroyPipelineLayout(device.Device(), m_PipelineLayout, nullptr);

        vkFreeCommandBuffers(device.Device(), VulkanContext::GetComputePool(), static_cast<u32>(m_CommandBuffers.size()), m_CommandBuffers.data());

        m_DescriptorSet.Shutdown();
    }

    void VulkanComputePipeline::Bind()
    {
        HE_PROFILE_FUNCTION();

        UpdateFrameIndex();

        HE_ENGINE_ASSERT(!m_SubmittedThisFrame, "Cannot bind a framebuffer that has already been submitted");

        VkCommandBuffer buffer = GetCommandBuffer();

        if (!m_BoundThisFrame)
        {
            m_BoundThisFrame = true;
     
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            HE_VULKAN_CHECK_RESULT(vkBeginCommandBuffer(buffer, &beginInfo));
        }
    }

    void VulkanComputePipeline::BindShaderBufferResource(u32 bindingIndex, u32 elementOffset, u32 elementCount, Buffer* buffer)
    {
        HE_PROFILE_FUNCTION();

        HE_ENGINE_ASSERT(elementCount + elementOffset <= buffer->GetAllocatedCount(), "ElementCount + ElementOffset must be <= buffer allocated count");
        HE_ENGINE_ASSERT(buffer->GetType() == Buffer::Type::Uniform || buffer->GetType() == Buffer::Type::Storage, "Buffer bind must be either a uniform or storage buffer");

        ShaderResourceType bufferType = buffer->GetType() == Buffer::Type::Uniform ? ShaderResourceType::UniformBuffer : ShaderResourceType::StorageBuffer;
        BindShaderResource(bindingIndex, bufferType, buffer, true, buffer->GetLayout().GetStride() * elementOffset, buffer->GetLayout().GetStride() * elementCount);
    }

    void VulkanComputePipeline::BindShaderTextureResource(u32 bindingIndex, Texture* texture)
    {
        HE_PROFILE_FUNCTION();
        
        BindShaderResource(bindingIndex, ShaderResourceType::Texture, texture, false, 0, 0);
    }

    void VulkanComputePipeline::BindShaderTextureLayerResource(u32 bindingIndex, Texture* texture, u32 layerIndex, u32 mipLevel)
    {
        HE_PROFILE_FUNCTION();

        BindShaderResource(bindingIndex, ShaderResourceType::Texture, texture, true, layerIndex, mipLevel);
    }

    void VulkanComputePipeline::BindShaderResource(u32 bindingIndex, ShaderResourceType resourceType, void* resource, bool useOffset, u32 offset, u32 size)
    {        
        if (!m_DescriptorSet.DoesBindingExist(bindingIndex))
            return; // silently ignore, TODO: warning once in the console when this happens

        HE_ENGINE_ASSERT(m_DescriptorSet.IsResourceCorrectType(bindingIndex, resourceType), "Attempting to bind a resource that does not match the bind index type");

        if (useOffset && (resourceType == ShaderResourceType::UniformBuffer || resourceType == ShaderResourceType::StorageBuffer))
            m_DescriptorSet.UpdateDynamicOffset(bindingIndex, offset);

        // update the descriptor set binding information
        m_DescriptorSet.UpdateShaderResource(bindingIndex, resourceType, resource, useOffset, offset, size);
    }

    void VulkanComputePipeline::FlushBindings()
    {
        // update a newly allocated descriptor set based on the current bindings or return
        // a cached one that was created before with the same binding info
        m_DescriptorSet.FlushBindings();

        HE_ENGINE_ASSERT(m_DescriptorSet.GetMostRecentDescriptorSet() != nullptr);

        // bind the new set
        VkDescriptorSet sets[1] = { m_DescriptorSet.GetMostRecentDescriptorSet() };
        vkCmdBindDescriptorSets(
            GetCommandBuffer(),
            VK_PIPELINE_BIND_POINT_COMPUTE,
            m_PipelineLayout,
            0, 1,
            sets,
            static_cast<u32>(m_DescriptorSet.GetDynamicOffsets().size()),
            m_DescriptorSet.GetDynamicOffsets().data()
        );

        m_FlushedThisFrame = true;
    }

    void VulkanComputePipeline::Submit()
    {
        HE_PROFILE_FUNCTION();

        if (!m_SubmittedThisFrame)
        {
            VulkanDevice& device = VulkanContext::GetDevice();
            VkCommandBuffer buffer = GetCommandBuffer();

            // dispatch
            vkCmdDispatch(
                buffer,
                std::min(m_DispatchCountX, device.PhysicalDeviceProperties().limits.maxComputeWorkGroupCount[0]),
                std::min(m_DispatchCountY, device.PhysicalDeviceProperties().limits.maxComputeWorkGroupCount[1]),
                std::min(m_DispatchCountZ, device.PhysicalDeviceProperties().limits.maxComputeWorkGroupCount[2])
            );

            // end the main command buffer
            HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(buffer));

            m_BoundThisFrame = false;
            m_SubmittedThisFrame = true;
            m_FlushedThisFrame = false;
        }
    }

    void VulkanComputePipeline::UpdateFrameIndex()
    {
        if (App::Get().GetFrameCount() != m_LastUpdateFrame)
        {
            // it is a new frame so we need to get the new flight frame index
            VulkanDevice& device = VulkanContext::GetDevice();
            VulkanContext& mainContext = static_cast<VulkanContext&>(Window::GetMainWindow().GetContext());

            m_SubmittedThisFrame = false;

            m_InFlightFrameIndex = mainContext.GetSwapChain().GetInFlightFrameIndex();
            m_LastUpdateFrame = App::Get().GetFrameCount();
        }
    }
}