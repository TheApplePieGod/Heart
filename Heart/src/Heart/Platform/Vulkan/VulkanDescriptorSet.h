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
        void Initialize(const std::vector<ReflectionDataElement>& reflectionData, const ShaderPreprocessData& preprocessData);
        void Shutdown();

        void UpdateShaderResource(u32 bindingIndex, ShaderResourceType resourceType, void* resource, bool useOffset, u32 offset); // offset used for image
        void FlushBindings();

        inline VkDescriptorSetLayout GetLayout() const { return m_DescriptorSetLayout; };
        inline VkDescriptorSet GetMostRecentDescriptorSet() const { return m_MostRecentDescriptorSet; }
        inline void UpdateDynamicOffset(u32 bindingIndex, u32 offset) { m_DynamicOffsets[m_Bindings[bindingIndex].OffsetIndex] = offset; }
        inline const std::vector<u32>& GetDynamicOffsets() const { return m_DynamicOffsets; }
        inline bool DoesBindingExist(u32 bindingIndex) const { return bindingIndex < m_Bindings.size(); }
        inline bool IsBindingDynamic(u32 bindingIndex) const { return m_Bindings[bindingIndex].IsDynamic; };
        inline bool CanFlush() const { return m_WritesReadyCount == m_CachedDescriptorWrites.size(); }

    private:
        struct BoundResource
        {
            void* Resource;
            u32 Offset;
        };
        struct BindingData
        {
            size_t DescriptorWriteMapping;
            bool IsDynamic;
            size_t OffsetIndex;
        };

    private:
        VkDescriptorPool CreateDescriptorPool();
        inline void PushDescriptorPool() { m_DescriptorPools[m_InFlightFrameIndex].emplace_back(CreateDescriptorPool()); }
        VkDescriptorSet AllocateSet();
        void ClearPools();
        size_t HashBindings();

    private:
        // TODO: parameterize?
        const u32 m_MaxSetsPerPool = 50;

        VkDescriptorSetLayout m_DescriptorSetLayout;
        std::vector<VkDescriptorSetLayout> m_CachedSetLayouts;
        std::array<std::vector<VkDescriptorPool>, MAX_FRAMES_IN_FLIGHT> m_DescriptorPools;
        VkDescriptorSet m_MostRecentDescriptorSet;

        std::vector<VkDescriptorSet> m_AvailableSets;
        std::vector<VkDescriptorPoolSize> m_CachedPoolSizes;
        std::vector<VkWriteDescriptorSet> m_CachedDescriptorWrites;
        std::array<VkDescriptorBufferInfo, MAX_UNIQUE_DESCRIPTORS> m_CachedBufferInfos;
        std::array<VkDescriptorImageInfo, MAX_DESCRIPTOR_ARRAY_COUNT * MAX_UNIQUE_DESCRIPTORS> m_CachedImageInfos;
        std::unordered_map<size_t, VkDescriptorSet> m_CachedDescriptorSets;

        u64 m_LastResetFrame = 0;
        u32 m_InFlightFrameIndex = 0;
        size_t m_WritesReadyCount = 0;
        size_t m_AvailableSetIndex = 0;
        size_t m_AvailablePoolIndex = 0;

        std::vector<BindingData> m_Bindings;
        std::unordered_map<u32, BoundResource> m_BoundResources;
        std::vector<u32> m_DynamicOffsets;
    };
}