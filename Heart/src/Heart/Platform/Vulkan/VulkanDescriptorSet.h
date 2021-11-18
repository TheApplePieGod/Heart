#pragma once

#include "Heart/Renderer/Shader.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Platform/Vulkan/VulkanSwapChain.h"

#define MAX_DESCRIPTOR_ARRAY_COUNT 20
#define MAX_UNIQUE_DESCRIPTORS 100

namespace Heart
{
    class VulkanDescriptorSet
    {
    public:
        void Initialize(const std::vector<ReflectionDataElement>& reflectionData);
        void Shutdown();

        // returns true if a call to vkCmdBindDescriptorSets using GetMostRecentDescriptorSet() is allowed
        bool UpdateShaderResource(u32 bindingIndex, ShaderResourceType resourceType, void* resource);

        inline VkDescriptorSetLayout GetLayout() const { return m_DescriptorSetLayout; };
        inline VkDescriptorSet GetMostRecentDescriptorSet() const { return m_MostRecentDescriptorSet; }
        inline void UpdateDynamicOffset(u32 bindingIndex, u32 offset) { m_DynamicOffsets[m_OffsetMappings[bindingIndex]] = offset; }
        inline const std::vector<u32>& GetDynamicOffsets() const { return m_DynamicOffsets; }
        inline bool DoesBindingExist(u32 bindingIndex) const { return m_DescriptorWriteMappings.find(bindingIndex) != m_DescriptorWriteMappings.end(); }

    private:
        VkDescriptorPool CreateDescriptorPool();
        inline void PushDescriptorPool() { m_DescriptorPools[m_InFlightFrameIndex].emplace_back(CreateDescriptorPool()); }
        VkDescriptorSet AllocateSet();
        void ClearPools();

    private:
        // TODO: parameterize?
        const u32 m_MaxSetsPerPool = 50;

        VkDescriptorSetLayout m_DescriptorSetLayout;
        std::array<std::vector<VkDescriptorPool>, MAX_FRAMES_IN_FLIGHT> m_DescriptorPools;
        VkDescriptorSet m_MostRecentDescriptorSet;

        std::vector<VkDescriptorPoolSize> m_CachedPoolSizes;
        std::vector<VkWriteDescriptorSet> m_CachedDescriptorWrites;
        std::array<VkDescriptorBufferInfo, MAX_UNIQUE_DESCRIPTORS> m_CachedBufferInfos;
        std::array<VkDescriptorImageInfo, MAX_DESCRIPTOR_ARRAY_COUNT * MAX_UNIQUE_DESCRIPTORS> m_CachedImageInfos;
        std::unordered_map<u32, size_t> m_DescriptorWriteMappings;

        u32 m_FrameDataRegistryId;
        u64 m_LastResetFrame = 0;
        u32 m_InFlightFrameIndex = 0;
        size_t m_LastSuccessfulPool = 0;
        size_t m_WritesReadyCount = 0;

        std::unordered_map<u32, void*> m_BoundResources;

        std::vector<u32> m_DynamicOffsets;
        std::unordered_map<u32, size_t> m_OffsetMappings;
    };
}