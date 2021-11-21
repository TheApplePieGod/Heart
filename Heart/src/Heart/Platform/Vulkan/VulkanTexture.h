#pragma once

#include "Heart/Renderer/Texture.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanTexture : public Texture
    {
    public:
        VulkanTexture(int width, int height, int channels, void* data, u32 arrayCount, bool floatComponents);
        ~VulkanTexture() override;

        inline VkImageView GetImageView() const { return m_ImageView; }
        inline VkImageView GetLayerImageView(u32 layerIndex) const { return m_LayerViews[layerIndex]; }
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
        const u32 m_DesiredMipLevels = 5;

        VkImage m_Image;
        VkImageView m_ImageView;
        VkDeviceMemory m_ImageMemory;
        VkFormat m_Format;
        ColorFormat m_GeneralFormat;
        std::vector<VkImageView> m_LayerViews;
        u32 m_MipLevels;
        VkImageLayout m_CurrentLayout;
        VkSampler m_Sampler;
    };
}