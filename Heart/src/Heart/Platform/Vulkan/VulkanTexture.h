#pragma once

#include "Heart/Renderer/Texture.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanTexture : public Texture
    {
    public:
        VulkanTexture(const TextureCreateInfo& createInfo, void* initialData);
        ~VulkanTexture() override;

        inline VkImageView GetImageView() const { return m_ImageView; }
        inline VkImageView GetLayerImageView(u32 layerIndex, u32 mipLevel) const { return m_LayerViews[layerIndex * m_MipLevels + mipLevel]; }
        inline VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }
        inline VkSampler GetSampler() const { return m_Sampler; }
        inline ColorFormat GetGeneralFormat() const { return m_GeneralFormat; }
        inline VkFormat GetFormat() const { return m_Format; }
        inline VkImage GetImage() const { return m_Image; }
        inline VkDeviceMemory GetImageMemory() const { return m_ImageMemory; }

    private:
        void CreateTexture(void* data);
        void CreateSampler();

    private:
        VkImage m_Image;
        VkImageView m_ImageView;
        VkDeviceMemory m_ImageMemory;
        VkFormat m_Format;
        ColorFormat m_GeneralFormat;
        std::vector<VkImageView> m_LayerViews;
        VkImageLayout m_CurrentLayout;
        VkSampler m_Sampler;
    };
}