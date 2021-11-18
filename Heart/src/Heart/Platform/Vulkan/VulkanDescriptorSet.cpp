#include "htpch.h"
#include "VulkanDescriptorSet.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanBuffer.h"
#include "Heart/Platform/Vulkan/VulkanTexture.h"
#include "Heart/Platform/Vulkan/VulkanFramebuffer.h"
#include "Heart/Core/Window.h"
#include "Heart/Core/App.h"

namespace Heart
{
    void VulkanDescriptorSet::Initialize(const std::vector<ReflectionDataElement>& reflectionData)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        // create the descriptor set layout and cache the associated poolsizes
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::unordered_map<VkDescriptorType, u32> descriptorCounts;
        for (auto& element : reflectionData)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = element.BindingIndex;
            binding.descriptorType = VulkanCommon::ShaderResourceTypeToVulkan(element.ResourceType);
            binding.descriptorCount = element.ArrayCount;
            binding.stageFlags = VulkanCommon::ShaderResourceAccessTypeToVulkan(element.AccessType);
            binding.pImmutableSamplers = nullptr;
            bindings.emplace_back(binding);

            descriptorCounts[binding.descriptorType] += element.ArrayCount * m_MaxSetsPerPool;

            // populated the cached descriptor writes for updating new sets
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstBinding = element.BindingIndex;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = binding.descriptorType;
            descriptorWrite.descriptorCount = element.ArrayCount;

            m_DescriptorWriteMappings[element.BindingIndex] = m_CachedDescriptorWrites.size(); 
            m_CachedDescriptorWrites.emplace_back(descriptorWrite);

            // populate the dynamic offset info if applicable
            if (element.ResourceType == ShaderResourceType::UniformBuffer || element.ResourceType == ShaderResourceType::StorageBuffer)
            {
                m_OffsetMappings[element.BindingIndex] = m_DynamicOffsets.size();
                m_DynamicOffsets.emplace_back(0);
            }
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<u32>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        HE_VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(device.Device(), &layoutInfo, nullptr, &m_DescriptorSetLayout));

        // populate cached layouts because one is needed for each allocation
        for (u32 i = 0; i < m_MaxSetsPerPool; i++)
            m_CachedSetLayouts.emplace_back(m_DescriptorSetLayout);

        // populate the cached pool sizes
        for (auto& element : descriptorCounts)
        {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = element.first;
            poolSize.descriptorCount = element.second;

            m_CachedPoolSizes.emplace_back(poolSize);
        }

        m_AvailableSets.resize(m_MaxSetsPerPool);

        // create the initial descriptor pools per frame
        for (size_t i = 0; i < m_DescriptorPools.size(); i++)
            m_DescriptorPools[i].push_back(CreateDescriptorPool());
    }

    void VulkanDescriptorSet::Shutdown()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext::Sync();

        for (size_t i = 0; i < m_DescriptorPools.size(); i++)
        {
            for (auto& pool : m_DescriptorPools[i])
            {
                vkDestroyDescriptorPool(device.Device(), pool, nullptr);
            }
        }

        vkDestroyDescriptorSetLayout(device.Device(), m_DescriptorSetLayout, nullptr);
    }

    bool VulkanDescriptorSet::UpdateShaderResource(u32 bindingIndex, ShaderResourceType resourceType, void* resource)
    {
        HE_PROFILE_FUNCTION();

        VulkanDevice& device = VulkanContext::GetDevice();
        if (App::Get().GetFrameCount() != m_LastResetFrame)
        {
            // it is a new frame so we need to clear out the descriptor pools
            ClearPools();
            m_LastResetFrame = App::Get().GetFrameCount();
            m_BoundResources.clear();
            
            m_WritesReadyCount = 0;
            for (auto& write : m_CachedDescriptorWrites)
            {
                write.pBufferInfo = nullptr;
                write.pImageInfo = nullptr;
            }
        }

        if (m_BoundResources[bindingIndex] == resource)
            return m_WritesReadyCount == m_CachedDescriptorWrites.size();
        m_BoundResources[bindingIndex] = resource;

        //HE_ENGINE_ASSERT(m_DescriptorWriteMappings.find(bindingIndex) != m_DescriptorWriteMappings.end(), "Attempting to update a shader resource binding that doesn't exist");

        u32 bufferInfoBaseIndex = bindingIndex;
        u32 imageInfoBaseIndex = bindingIndex * MAX_DESCRIPTOR_ARRAY_COUNT;
        switch (resourceType)
        {
            default: { HE_ENGINE_ASSERT(false, "Cannot update VulkanDescriptorSet with selected resource type"); } break;

            case ShaderResourceType::UniformBuffer:
            case ShaderResourceType::StorageBuffer:
            {
                VulkanBuffer* buffer = static_cast<VulkanBuffer*>(resource);
                HE_ENGINE_ASSERT(bindingIndex < m_CachedBufferInfos.size(), "Binding index for buffer resource is too large");

                m_CachedBufferInfos[bufferInfoBaseIndex].buffer = buffer->GetBuffer();
                m_CachedBufferInfos[bufferInfoBaseIndex].offset = 0;
                m_CachedBufferInfos[bufferInfoBaseIndex].range = buffer->GetLayout().GetStride(); //buffer.GetAllocatedSize(); // when using dynamic buffers, range is the stride rather than the whole size
            } break;

            case ShaderResourceType::Texture:
            {
                VulkanTexture* texture = static_cast<VulkanTexture*>(resource);
                HE_ENGINE_ASSERT(texture->GetArrayCount() <= MAX_DESCRIPTOR_ARRAY_COUNT, "Image array count too large");

                for (u32 i = 0; i < texture->GetArrayCount(); i++)
                {
                    // TODO: customizable sampler
                    m_CachedImageInfos[imageInfoBaseIndex + i].sampler = VulkanContext::GetDefaultSampler();
                    m_CachedImageInfos[imageInfoBaseIndex + i].imageLayout = texture->GetCurrentLayout();
                    m_CachedImageInfos[imageInfoBaseIndex + i].imageView = texture->GetImageView();
                }
            } break;

            case ShaderResourceType::SubpassInput:
            {
                VulkanFramebuffer::VulkanFramebufferAttachment* attachment = static_cast<VulkanFramebuffer::VulkanFramebufferAttachment*>(resource);
                HE_ENGINE_ASSERT(bindingIndex < m_CachedImageInfos.size(), "Binding index for subpass input is too large");

                m_CachedImageInfos[imageInfoBaseIndex].sampler = NULL;
                m_CachedImageInfos[imageInfoBaseIndex].imageLayout = attachment->IsDepthAttachment ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                m_CachedImageInfos[imageInfoBaseIndex].imageView = attachment->HasResolve ? attachment->ResolveImageView : attachment->ImageView;
            } break;
        }

        VkWriteDescriptorSet& descriptorWrite = m_CachedDescriptorWrites[m_DescriptorWriteMappings[bindingIndex]];
        if (descriptorWrite.pBufferInfo == nullptr && descriptorWrite.pImageInfo == nullptr)
            m_WritesReadyCount++;

        descriptorWrite.pBufferInfo = &m_CachedBufferInfos[bufferInfoBaseIndex];
        descriptorWrite.pImageInfo = &m_CachedImageInfos[imageInfoBaseIndex];

        // in order to keep vulkan happy, we cannot update a descriptor set 
        if (m_WritesReadyCount == m_CachedDescriptorWrites.size())
        {
            m_MostRecentDescriptorSet = AllocateSet();
            for (auto& write : m_CachedDescriptorWrites)
                write.dstSet = m_MostRecentDescriptorSet;

            vkUpdateDescriptorSets(device.Device(), static_cast<u32>(m_CachedDescriptorWrites.size()), m_CachedDescriptorWrites.data(), 0, nullptr);

            return true;
        }

        return false;
    }

    VkDescriptorPool VulkanDescriptorSet::CreateDescriptorPool()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<u32>(m_CachedPoolSizes.size());
        poolInfo.pPoolSizes = m_CachedPoolSizes.data();
        poolInfo.maxSets = m_MaxSetsPerPool;
        poolInfo.flags = 0;

        VkDescriptorPool pool;
        HE_VULKAN_CHECK_RESULT(vkCreateDescriptorPool(device.Device(), &poolInfo, nullptr, &pool));

        return pool;
    }

    // TODO: last successful index to speed up allocation when searching for a pool
    VkDescriptorSet VulkanDescriptorSet::AllocateSet()
    {
        // generate if we go over the size limit or if this is the first allocation of the frame
        if (m_AvailablePoolIndex == 0 || m_AvailableSetIndex > m_AvailableSets.size())
        {
            m_AvailableSetIndex = 0;

            if (m_AvailablePoolIndex >= m_DescriptorPools[m_InFlightFrameIndex].size())
                PushDescriptorPool();

            VulkanDevice& device = VulkanContext::GetDevice();
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_DescriptorPools[m_InFlightFrameIndex][m_AvailablePoolIndex++];
            allocInfo.descriptorSetCount = m_MaxSetsPerPool;
            allocInfo.pSetLayouts = m_CachedSetLayouts.data();

            VkResult result = vkAllocateDescriptorSets(device.Device(), &allocInfo, m_AvailableSets.data());
        }
        return m_AvailableSets[m_AvailableSetIndex++];
    }

    void VulkanDescriptorSet::ClearPools()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext& mainContext = static_cast<VulkanContext&>(Window::GetMainWindow().GetContext()); // we need the main context here to sync the inflightframeindex

        m_InFlightFrameIndex = mainContext.GetSwapChain().GetInFlightFrameIndex();
        m_AvailableSetIndex = 0;
        m_AvailablePoolIndex = 0;

        for (auto& pool : m_DescriptorPools[m_InFlightFrameIndex])
            vkResetDescriptorPool(device.Device(), pool, 0);
    }
}