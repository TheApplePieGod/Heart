#include "htpch.h"
#include "VulkanShaderInput.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanBuffer.h"
#include "Heart/Platform/Vulkan/VulkanTexture.h"
#include "Heart/Core/Window.h"
#include "Heart/Core/App.h"

namespace Heart
{
    VulkanShaderInputSet::VulkanShaderInputSet(std::initializer_list<ShaderInputElement> elements)
        : ShaderInputSet(elements)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        // create the descriptor set layout and cache the associated poolsizes
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::unordered_map<VkDescriptorType, u32> descriptorCounts;
        for (auto& element : elements)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = element.BindIndex;
            binding.descriptorType = VulkanCommon::ShaderInputTypeToVulkan(element.InputType);
            binding.descriptorCount = element.ArrayCount;
            binding.stageFlags = VulkanCommon::ShaderBindTypeToVulkan(element.BindType);
            binding.pImmutableSamplers = nullptr;

            bindings.emplace_back(binding);

            if (descriptorCounts.find(binding.descriptorType) == descriptorCounts.end())
                descriptorCounts[binding.descriptorType] = element.ArrayCount * m_MaxSetsPerPool;
            else
                descriptorCounts[binding.descriptorType] += element.ArrayCount * m_MaxSetsPerPool;

            // populated the cached descriptor writes for updating new sets
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstBinding = element.BindIndex;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = binding.descriptorType;
            descriptorWrite.descriptorCount = element.ArrayCount;

            m_CachedDescriptorWrites.emplace_back(descriptorWrite);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<u32>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        HE_VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(device.Device(), &layoutInfo, nullptr, &m_DescriptorSetLayout));

        // populate the cached pool sizes
        for (auto& element : descriptorCounts)
        {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = element.first;
            poolSize.descriptorCount = element.second;

            m_CachedPoolSizes.emplace_back(poolSize);
        }

        // create the initial descriptor pools per frame
        for (size_t i = 0; i < m_DescriptorPools.size(); i++)
            m_DescriptorPools[i].push_back(CreateDescriptorPool());
    }

    VulkanShaderInputSet::~VulkanShaderInputSet()
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

    ShaderInputBindPoint VulkanShaderInputSet::CreateBindPoint(const std::vector<ShaderInputBindElement>& bindElements)
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        if (App::Get().GetFrameCount() != m_LastResetFrame)
        {
            // it is a new frame so we need to clear out the descriptor pools
            ClearPools();
            m_LastResetFrame = App::Get().GetFrameCount();
        }

        if (bindElements.size() != m_Elements.size())
            HE_ENGINE_LOG_WARN("Binding of size {0} being bound to descriptor set size {1}", bindElements.size(), m_Elements.size());

        VkDescriptorSet set = AllocateSet();

        size_t index = 0;
        size_t bufferIndex = 0;
        size_t imageIndex = 0;
        for (auto& element : bindElements)
        {
            if (index >= m_CachedDescriptorWrites.size()) // m_CachedDescriptorWrites should have the same size as m_Elements
                break;

            m_CachedDescriptorWrites[index].dstSet = set;

            if (element.TargetBuffer != nullptr)
            {
                VulkanBuffer& buffer = static_cast<VulkanBuffer&>(*element.TargetBuffer.get());

                if (bufferIndex >= m_CachedBufferInfos.size())
                    m_CachedBufferInfos.emplace_back();

                m_CachedBufferInfos[bufferIndex].buffer = buffer.GetBuffer();
                m_CachedBufferInfos[bufferIndex].offset = 0;
                m_CachedBufferInfos[bufferIndex].range = buffer.GetAllocatedSize();

                m_CachedDescriptorWrites[index].pBufferInfo = &m_CachedBufferInfos[bufferIndex++];
            }
            else if (element.TargetTexture != nullptr)
            {
                VulkanTexture& texture = static_cast<VulkanTexture&>(*element.TargetTexture.get());

                size_t imageIndexBegin = imageIndex;
                for (u32 i = 0; i < m_Elements[index].ArrayCount; i++)
                {
                    if (imageIndex >= m_CachedImageInfos.size())
                        m_CachedImageInfos.emplace_back();

                    // TODO: customizable sampler
                    m_CachedImageInfos[imageIndex].sampler = VulkanContext::GetDefaultSampler();
                    m_CachedImageInfos[imageIndex].imageLayout = texture.GetCurrentLayout();
                    m_CachedImageInfos[imageIndex++].imageView = texture.GetImageView();
                }

                m_CachedDescriptorWrites[index].pImageInfo = &m_CachedImageInfos[imageIndexBegin];
            }

            index++;
        }

        vkUpdateDescriptorSets(device.Device(), static_cast<u32>(m_CachedDescriptorWrites.size()), m_CachedDescriptorWrites.data(), 0, nullptr);

        return static_cast<ShaderInputBindPoint>(set);
    }

    VkDescriptorPool VulkanShaderInputSet::CreateDescriptorPool()
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

    VkDescriptorSet VulkanShaderInputSet::AllocateSet()
    {
        size_t poolIndex = m_LastSuccessfulPool;

        VulkanDevice& device = VulkanContext::GetDevice();
        VkDescriptorSet set;
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPools[m_InFlightFrameIndex][poolIndex++];
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_DescriptorSetLayout;

        VkResult result = vkAllocateDescriptorSets(device.Device(), &allocInfo, &set);

        // TODO: possible infinite loop checking?
        while (result != VK_SUCCESS)
        {
            if (poolIndex >= m_DescriptorPools[m_InFlightFrameIndex].size())
                PushDescriptorPool();

            allocInfo.descriptorPool = m_DescriptorPools[m_InFlightFrameIndex][poolIndex++];

            result = vkAllocateDescriptorSets(device.Device(), &allocInfo, &set);
        }

        m_LastSuccessfulPool = poolIndex - 1;
        return set;
    }

    void VulkanShaderInputSet::ClearPools()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext& mainContext = static_cast<VulkanContext&>(Window::GetMainWindow().GetContext()); // we need the main context here to sync the inflightframeindex

        m_InFlightFrameIndex = mainContext.GetSwapChain().GetInFlightFrameIndex();
        m_LastSuccessfulPool = 0;

        for (auto& pool : m_DescriptorPools[m_InFlightFrameIndex])
            vkResetDescriptorPool(device.Device(), pool, 0);
    }
}