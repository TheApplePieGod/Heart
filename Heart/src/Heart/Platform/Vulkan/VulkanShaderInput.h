#pragma once

#include "Heart/Renderer/ShaderInput.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Platform/Vulkan/VulkanSwapChain.h"

namespace Heart
{
    class VulkanShaderInputSet : public ShaderInputSet
    {
    public:
        VulkanShaderInputSet(std::initializer_list<ShaderInputElement> elements);
        ~VulkanShaderInputSet() override;

        ShaderInputBindPoint CreateBindPoint(const std::vector<ShaderInputBindElement>& bindElements) override; // this will return a VkDescriptorSet

        inline VkDescriptorSetLayout GetLayout() const { return m_DescriptorSetLayout; };

    private:
        VkDescriptorPool CreateDescriptorPool();
        inline void PushDescriptorPool() { m_DescriptorPools[m_InFlightFrameIndex].emplace_back(CreateDescriptorPool()); }
        VkDescriptorSet AllocateSet();
        void ClearPools();

    private:
        // TODO: parameterize?
        const u32 m_MaxSetsPerPool = 50;

        VkDescriptorSetLayout m_DescriptorSetLayout;
        std::vector<VkDescriptorPoolSize> m_CachedPoolSizes;
        std::vector<VkWriteDescriptorSet> m_CachedDescriptorWrites;
        std::vector<VkDescriptorBufferInfo> m_CachedBufferInfos;
        std::vector<VkDescriptorImageInfo> m_CachedImageInfos;
        std::array<std::vector<VkDescriptorPool>, MAX_FRAMES_IN_FLIGHT> m_DescriptorPools;
        u32 m_FrameDataRegistryId;
        u64 m_LastResetFrame = 0;
        u32 m_InFlightFrameIndex = 0;
        size_t m_LastSuccessfulPool = 0;
    };
}