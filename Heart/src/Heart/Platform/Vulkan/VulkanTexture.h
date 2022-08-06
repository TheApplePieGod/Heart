#pragma once

#include "Heart/Renderer/Texture.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanBuffer;
    class VulkanTexture : public Texture
    {
    public:
        VulkanTexture(const TextureCreateInfo& createInfo, void* initialData);
        ~VulkanTexture() override;

        void RegenerateMipMaps() override;
        void RegenerateMipMapsSync(Framebuffer* buffer) override;

        void* GetPixelData() override;

        void* GetImGuiHandle(u32 layerIndex = 0, u32 mipLevel = 0) override;

        inline VkImageView GetImageView() { UpdateFrameIndex(); return m_ImageViews[m_InFlightFrameIndex]; }
        inline VkImageView GetImageView(u32 inFlightFrameIndex) { return m_ImageViews[std::min(m_ImageCount - 1, inFlightFrameIndex)]; }
        inline VkImageView GetLayerImageView(u32 layerIndex, u32 mipLevel) { UpdateFrameIndex(); return m_LayerViews[m_InFlightFrameIndex][layerIndex * m_MipLevels + mipLevel]; }
        inline VkImageView GetLayerImageView(u32 inFlightFrameIndex, u32 layerIndex, u32 mipLevel) { return m_LayerViews[std::min(m_ImageCount - 1, inFlightFrameIndex)][layerIndex * m_MipLevels + mipLevel]; }
        inline VkSampler GetSampler() const { return m_Sampler; }
        inline ColorFormat GetGeneralFormat() const { return m_GeneralFormat; }
        inline VkFormat GetFormat() const { return m_Format; }
        inline VkImage GetImage() { UpdateFrameIndex(); return m_Images[m_InFlightFrameIndex]; }
        inline VkImage GetImage(u32 inFlightFrameIndex) { return m_Images[std::min(m_ImageCount - 1, inFlightFrameIndex)]; }
        inline VkDeviceMemory GetImageMemory() { UpdateFrameIndex(); return m_ImageMemory[m_InFlightFrameIndex]; }
        inline VkDeviceMemory GetImageMemory(u32 inFlightFrameIndex) { return m_ImageMemory[std::min(m_ImageCount - 1, inFlightFrameIndex)]; }
        inline Ref<VulkanBuffer> GetCpuBuffer() const { return m_CpuBuffer; }

    private:
        void CreateTexture(void* data);
        void CreateSampler();
        void UpdateFrameIndex();

    private:
        std::array<VkImage, Renderer::FrameBufferCount> m_Images;
        std::array<VkImageView, Renderer::FrameBufferCount> m_ImageViews;
        std::array<VkDeviceMemory, Renderer::FrameBufferCount> m_ImageMemory;
        std::array<HVector<void*>, Renderer::FrameBufferCount> m_ImGuiHandles;
        VkFormat m_Format;
        ColorFormat m_GeneralFormat;
        std::array<HVector<VkImageView>, Renderer::FrameBufferCount> m_LayerViews;
        VkSampler m_Sampler;
        Ref<VulkanBuffer> m_CpuBuffer;

        s64 m_DataSize = 0;
        u64 m_LastUpdateFrame = 0;
        u32 m_InFlightFrameIndex = 0;
        u32 m_ImageCount = 0;
    };
}