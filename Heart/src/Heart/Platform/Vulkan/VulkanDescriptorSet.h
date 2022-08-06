#pragma once

#include "Heart/Events/EventEmitter.h"
#include "Heart/Renderer/Shader.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Container/HVector.hpp"

#define MAX_DESCRIPTOR_ARRAY_COUNT 20
#define MAX_UNIQUE_DESCRIPTORS 100

namespace Heart
{
    class TextureDeletedEvent;
    class VulkanDescriptorSet : public EventListener
    {
    public:
        void Initialize(const HVector<ReflectionDataElement>& reflectionData);
        void Shutdown();

        void UpdateShaderResource(u32 bindingIndex, ShaderResourceType resourceType, void* resource, bool useOffset, u32 offset, u32 size);
        void FlushBindings();

        void OnEvent(Event& event) override;

        inline VkDescriptorSetLayout GetLayout() const { return m_DescriptorSetLayout; };
        inline VkDescriptorSet GetMostRecentDescriptorSet() const { return m_MostRecentDescriptorSet; }
        inline void UpdateDynamicOffset(u32 bindingIndex, u32 offset) { m_DynamicOffsets[m_Bindings[bindingIndex].OffsetIndex] = offset; }
        inline const HVector<u32>& GetDynamicOffsets() const { return m_DynamicOffsets; }
        inline bool DoesBindingExist(u32 bindingIndex) const { return bindingIndex < m_Bindings.GetCount() && m_Bindings[bindingIndex].Exists; }
        inline bool IsResourceCorrectType(u32 bindingIndex, ShaderResourceType resourceType) const { return VulkanCommon::ShaderResourceTypeToVulkan(resourceType) == m_CachedDescriptorWrites[m_Bindings[bindingIndex].DescriptorWriteMapping].descriptorType; }
        inline bool CanFlush() const { return m_WritesReadyCount == m_CachedDescriptorWrites.GetCount(); }

    private:
        struct BoundResource
        {
            void* Resource;
            u32 Offset;
            u32 Size;
        };
        struct BindingData
        {
            bool Exists = false;
            size_t DescriptorWriteMapping = 0;
            size_t OffsetIndex = 0;
        };

    private:
        VkDescriptorPool CreateDescriptorPool();
        inline void PushDescriptorPool() { m_DescriptorPools[m_InFlightFrameIndex].AddInPlace(CreateDescriptorPool()); }
        VkDescriptorSet AllocateSet();
        void ClearPools(u32 inFlightFrameIndex);
        u64 HashBindings();
        bool OnTextureDeleted(TextureDeletedEvent& event);

    private:
        // TODO: parameterize?
        const u32 m_MaxSetsPerPool = 50;

        VkDescriptorSetLayout m_DescriptorSetLayout;
        HVector<VkDescriptorSetLayout> m_CachedSetLayouts;
        std::array<HVector<VkDescriptorPool>, Renderer::FrameBufferCount> m_DescriptorPools;
        VkDescriptorSet m_MostRecentDescriptorSet;

        std::array<HVector<VkDescriptorSet>, Renderer::FrameBufferCount> m_AvailableSets;
        HVector<VkDescriptorPoolSize> m_CachedPoolSizes;
        HVector<VkWriteDescriptorSet> m_CachedDescriptorWrites;
        std::array<VkDescriptorBufferInfo, MAX_UNIQUE_DESCRIPTORS> m_CachedBufferInfos;
        std::array<VkDescriptorImageInfo, MAX_DESCRIPTOR_ARRAY_COUNT * MAX_UNIQUE_DESCRIPTORS> m_CachedImageInfos;
        std::array<std::unordered_map<u64, VkDescriptorSet>, Renderer::FrameBufferCount> m_CachedDescriptorSets;

        u64 m_LastResetFrame = 0;
        u32 m_InFlightFrameIndex = 0;
        size_t m_WritesReadyCount = 0;
        std::array<size_t, Renderer::FrameBufferCount> m_AvailableSetIndex{};
        std::array<size_t, Renderer::FrameBufferCount> m_AvailablePoolIndex{};

        HVector<BindingData> m_Bindings;
        std::unordered_map<u32, BoundResource> m_BoundResources;
        HVector<u32> m_DynamicOffsets;
    };
}