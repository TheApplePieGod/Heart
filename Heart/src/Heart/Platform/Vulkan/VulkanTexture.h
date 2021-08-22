#pragma once

#include "Heart/Renderer/Texture.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanTexture : public Texture
    {
    public:
        VulkanTexture(const std::string& path);
        ~VulkanTexture() override;

        inline VkImageView GetImageView() const { return m_ImageView; }
        inline VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }

    private:
        void CreateTexture(void* data);

    private:
        const int m_DesiredChannelCount = 4; // all images will load as RGBA
        VkImage m_Image;
        VkImageView m_ImageView;
        VkDeviceMemory m_ImageMemory;
        VkImageLayout m_CurrentLayout;
    };
}