#pragma once

#include "Heart/Renderer/Texture.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanTexture : public Texture
    {
    public:
        VulkanTexture(const std::string& path, int width, int height, int channels, void* data);
        ~VulkanTexture() override;

        inline VkImageView GetImageView() const { return m_ImageView; }
        inline VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }

    private:
        void CreateTexture(void* data);

    private:
        VkImage m_Image;
        VkImageView m_ImageView;
        VkDeviceMemory m_ImageMemory;
        VkImageLayout m_CurrentLayout;
    };
}