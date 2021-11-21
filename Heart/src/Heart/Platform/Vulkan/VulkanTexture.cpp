#include "htpch.h"
#include "VulkanTexture.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "stb_image/stb_image.h"
#include "imgui/backends/imgui_impl_vulkan.h"

namespace Heart
{
    VulkanTexture::VulkanTexture(int width, int height, int channels, void* data, u32 arrayCount, bool floatComponents)
        : Texture(width, height, channels, arrayCount, floatComponents)
    {
        if (data != nullptr)
            ScanForTransparency(width, height, channels, data);
        CreateTexture(data);
    }

    VulkanTexture::~VulkanTexture()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext::Sync();

        vkDestroySampler(device.Device(), m_Sampler, nullptr);

        for (auto handle : m_LayerImGuiHandles)
            ImGui_ImplVulkan_RemoveTexture(handle);

        for (auto view : m_LayerViews)
            vkDestroyImageView(device.Device(), view, nullptr);

        vkDestroyImageView(device.Device(), m_ImageView, nullptr);
        vkDestroyImage(device.Device(), m_Image, nullptr);
        vkFreeMemory(device.Device(), m_ImageMemory, nullptr);
    }

    void VulkanTexture::CreateTexture(void* data)
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkDeviceSize imageSize = m_Width * m_Height * m_Channels;
        m_Format = m_FloatComponents ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM;
        m_GeneralFormat = m_FloatComponents ? ColorFormat::RGBA32F : ColorFormat::RGBA8;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        m_MipLevels = m_DesiredMipLevels;
        u32 maxMipLevels = static_cast<u32>(floor(log2(std::max(m_Width, m_Height)))) + 1;
        if (m_MipLevels > maxMipLevels)
            m_MipLevels = maxMipLevels;

        // create image & staging buffer and transfer the data into the first layer
        if (data != nullptr)
        {
            VulkanCommon::CreateBuffer(
                device.Device(),
                device.PhysicalDevice(),
                imageSize * (m_FloatComponents ? sizeof(float) : sizeof(unsigned char)),
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer,
                stagingBufferMemory
            );
            VulkanCommon::MapAndWriteBufferMemory(
                device.Device(),
                data,
                m_FloatComponents ? sizeof(float) : sizeof(unsigned char),
                static_cast<u32>(imageSize),
                stagingBufferMemory,
                0
            );
        }
        VulkanCommon::CreateImage(
            device.Device(),
            device.PhysicalDevice(),
            m_Width, m_Height,
            m_Format,
            m_MipLevels,
            m_ArrayCount,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_Image, m_ImageMemory,
            VK_IMAGE_LAYOUT_UNDEFINED
        );
        VulkanCommon::TransitionImageLayout(
            device.Device(),
            VulkanContext::GetTransferPool(),
            device.TransferQueue(),
            m_Image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            m_MipLevels,
            m_ArrayCount
        );
        if (data != nullptr)
        {
            VulkanCommon::CopyBufferToImage(
                device.Device(),
                VulkanContext::GetTransferPool(),
                device.TransferQueue(),
                stagingBuffer,
                m_Image,
                static_cast<u32>(m_Width), static_cast<u32>(m_Height)
            );
            VulkanCommon::GenerateMipmaps(
                device.Device(),
                device.PhysicalDevice(),
                VulkanContext::GetGraphicsPool(),
                device.GraphicsQueue(),
                m_Image,
                m_Format,
                m_Width, m_Height, 
                m_MipLevels,
                m_ArrayCount
            );
        }

        // destroy the staging buffer
        if (data != nullptr)
        {
            vkDestroyBuffer(device.Device(), stagingBuffer, nullptr);
            vkFreeMemory(device.Device(), stagingBufferMemory, nullptr);
        }

        CreateSampler();

        // generate the general image view & one for each layer
        m_ImageView = VulkanCommon::CreateImageView(device.Device(), m_Image, m_Format, m_MipLevels, m_ArrayCount, 0, VK_IMAGE_ASPECT_COLOR_BIT);
        for (u32 i = 0; i < m_ArrayCount; i++)
        {
            VkImageView layerView = VulkanCommon::CreateImageView(device.Device(), m_Image, m_Format, 1, 1, i, VK_IMAGE_ASPECT_COLOR_BIT);
            m_LayerViews.emplace_back(layerView);
            m_LayerImGuiHandles.emplace_back(ImGui_ImplVulkan_AddTexture(m_Sampler, layerView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        }

        m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    void VulkanTexture::CreateSampler()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device.PhysicalDevice(), &properties);
        
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VulkanCommon::SamplerFilterToVulkan(m_SamplerState.MagFilter);
        samplerInfo.minFilter = VulkanCommon::SamplerFilterToVulkan(m_SamplerState.MinFilter);
        samplerInfo.addressModeU = VulkanCommon::SamplerWrapModeToVulkan(m_SamplerState.UVWWrap[0]);
        samplerInfo.addressModeV = VulkanCommon::SamplerWrapModeToVulkan(m_SamplerState.UVWWrap[1]);
        samplerInfo.addressModeW = VulkanCommon::SamplerWrapModeToVulkan(m_SamplerState.UVWWrap[2]);
        samplerInfo.anisotropyEnable = m_SamplerState.AnisotropyEnable;
        samplerInfo.maxAnisotropy = std::min(static_cast<float>(m_SamplerState.MaxAnisotropy), properties.limits.maxSamplerAnisotropy);
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(m_MipLevels);

        HE_VULKAN_CHECK_RESULT(vkCreateSampler(device.Device(), &samplerInfo, nullptr, &m_Sampler));
    }
}