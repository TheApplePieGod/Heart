#pragma once

#include "Heart/Renderer/Cubemap.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanCubemap : public Cubemap
    {
    public:
        VulkanCubemap(int width, int height, bool floatComponents);
        ~VulkanCubemap() override;

        inline VkImageView GetImageView() const { return m_ImageView; }
        inline VkImageView GetFaceImageView(u32 faceIndex) const { return m_FaceImageViews[faceIndex]; }
        inline VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }
        inline VkSampler GetSampler() const { return m_Sampler; }

    private:
        void CreateCubemap();
        void CreateSampler();

    private:
        const u32 m_DesiredMipLevels = 5;

        u32 m_MipLevels;
        VkImage m_Image;
        VkImageView m_ImageView;
        std::array<VkImageView, 6> m_FaceImageViews;
        VkDeviceMemory m_ImageMemory;
        VkImageLayout m_CurrentLayout;
        VkSampler m_Sampler;
    };
}