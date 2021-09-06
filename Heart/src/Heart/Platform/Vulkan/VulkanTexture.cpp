#include "htpch.h"
#include "VulkanTexture.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "stb_image/stb_image.h"

namespace Heart
{
    VulkanTexture::VulkanTexture(const std::string& path)
        : Texture(path)
    {
        unsigned char* pixels = stbi_load(path.c_str(), &m_Width, &m_Height, &m_Channels, m_DesiredChannelCount);
        if (pixels == nullptr)
        {
            HE_ENGINE_LOG_ERROR("Failed to load image at path {0}", path);
            HE_ENGINE_ASSERT(false);
        }
        HE_ENGINE_LOG_TRACE("Texture info: {0}x{1} w/ {2} channels", m_Width, m_Height, m_Channels);
        
        CreateTexture(pixels);

        stbi_image_free(pixels);
    }

    VulkanTexture::~VulkanTexture()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext::Sync();

        vkDestroyImageView(device.Device(), m_ImageView, nullptr);
        vkDestroyImage(device.Device(), m_Image, nullptr);
        vkFreeMemory(device.Device(), m_ImageMemory, nullptr);
    }

    void VulkanTexture::CreateTexture(void* data)
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkDeviceSize imageSize = static_cast<u64>(m_Width * m_Height * m_DesiredChannelCount);

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        VulkanCommon::MapAndWriteBufferMemory(device.Device(), data, sizeof(unsigned char), static_cast<u32>(imageSize), stagingBufferMemory, 0);

        VulkanCommon::CreateImage(device.Device(), device.PhysicalDevice(), m_Width, m_Height, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_ImageMemory);

        VulkanCommon::TransitionImageLayout(device.Device(), VulkanContext::GetTransferPool(), device.TransferQueue(), m_Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VulkanCommon::CopyBufferToImage(device.Device(), VulkanContext::GetTransferPool(), device.TransferQueue(), stagingBuffer, m_Image, static_cast<u32>(m_Width), static_cast<u32>(m_Height));

        VulkanCommon::TransitionImageLayout(device.Device(), VulkanContext::GetTransferPool(), device.TransferQueue(), m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(device.Device(), stagingBuffer, nullptr);
        vkFreeMemory(device.Device(), stagingBufferMemory, nullptr);

        m_ImageView = VulkanCommon::CreateImageView(device.Device(), m_Image, VK_FORMAT_R8G8B8A8_UNORM, 1);
        m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}