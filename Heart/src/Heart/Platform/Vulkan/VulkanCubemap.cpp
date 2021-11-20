#include "htpch.h"
#include "VulkanCubemap.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "stb_image/stb_image.h"
#include "imgui/backends/imgui_impl_vulkan.h"

namespace Heart
{
    VulkanCubemap::VulkanCubemap(int width, int height, bool floatComponents)
        : Cubemap(width, height, floatComponents)
    {
        // ScanForTransparency(width, height, channels, data);
        CreateCubemap();

        // if (load)
        //     stbi_image_free(data);
    }

    VulkanCubemap::~VulkanCubemap()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext::Sync();

        vkDestroySampler(device.Device(), m_Sampler, nullptr);

        //ImGui_ImplVulkan_RemoveTexture(m_ImGuiHandle);

        vkDestroyImageView(device.Device(), m_ImageView, nullptr);
        vkDestroyImage(device.Device(), m_Image, nullptr);
        vkFreeMemory(device.Device(), m_ImageMemory, nullptr);
    }

    void VulkanCubemap::CreateCubemap()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkDeviceSize imageSize = static_cast<u64>(m_Width * m_Height * m_DesiredChannelCount * 6); // 6 faces
        VkFormat format = m_FloatComponents ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM;

        m_MipLevels = m_DesiredMipLevels;
        u32 maxMipLevels = static_cast<u32>(floor(log2(std::max(m_Width, m_Height)))) + 1;
        if (m_MipLevels > maxMipLevels)
            m_MipLevels = maxMipLevels;

        // TODO: cubemap mip levels
        m_MipLevels = 1;

        VulkanCommon::CreateImage(device.Device(), device.PhysicalDevice(), m_Width, m_Height, format, m_MipLevels, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_ImageMemory, VK_IMAGE_LAYOUT_UNDEFINED, true);

        VulkanCommon::TransitionImageLayout(device.Device(), VulkanContext::GetTransferPool(), device.TransferQueue(), m_Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels, true);

        VulkanCommon::GenerateMipmaps(device.Device(), device.PhysicalDevice(), VulkanContext::GetGraphicsPool(), device.GraphicsQueue(), m_Image, format, m_Width, m_Height, m_MipLevels);
        //VulkanCommon::TransitionImageLayout(device.Device(), VulkanContext::GetTransferPool(), device.TransferQueue(), m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_MipLevels, true);

        CreateSampler();

        m_ImageView = VulkanCommon::CreateImageView(device.Device(), m_Image, format, m_MipLevels, 0, VK_IMAGE_ASPECT_COLOR_BIT, true);
        for (u32 i = 0; i < 6; i++)
        {
            m_FaceImageViews[i] = VulkanCommon::CreateImageView(device.Device(), m_Image, format, m_MipLevels, i, VK_IMAGE_ASPECT_COLOR_BIT, false);
        }

        m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        //m_ImGuiHandle = ImGui_ImplVulkan_AddTexture(m_Sampler, m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void VulkanCubemap::CreateSampler()
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