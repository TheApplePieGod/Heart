#pragma once

#include "Heart/Renderer/Texture.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanTexture : public Texture
    {
    public:
        VulkanTexture(const std::string& path, bool floatComponents, int width, int height, int channels, void* data);
        ~VulkanTexture() override;

        inline VkImageView GetImageView() const { return m_ImageView; }
        inline VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }
        inline VkSampler GetSampler() const { return m_Sampler; }

    private:
        void CreateTexture(void* data);
        void CreateSampler();

    private:
        const u32 m_DesiredMipLevels = 5;

        u32 m_MipLevels;
        VkImage m_Image;
        VkImageView m_ImageView;
        VkDeviceMemory m_ImageMemory;
        VkImageLayout m_CurrentLayout;
        VkSampler m_Sampler;
    };
}