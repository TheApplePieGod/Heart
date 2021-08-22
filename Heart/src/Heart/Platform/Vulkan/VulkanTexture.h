#pragma once

#include "Heart/Renderer/Texture.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanTexture : public Texture
    {
    public:

        inline VkImageView GetImageView() const { return m_ImageView; }
        inline VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }

    private:
        VkImage m_Image;
        VkImageView m_ImageView;
        VkDeviceMemory m_ImageMemory;
        VkImageLayout m_CurrentLayout;
    };
}