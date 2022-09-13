#include "hepch.h"
#include "VulkanTexture.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"
#include "Heart/Events/GraphicsEvents.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanFramebuffer.h"
#include "Heart/Platform/Vulkan/VulkanBuffer.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "stb_image/stb_image.h"
#include "imgui/backends/imgui_impl_vulkan.h"

namespace Heart
{
    VulkanTexture::VulkanTexture(const TextureCreateInfo& createInfo, void* initialData)
        : Texture(createInfo)
    {
        HE_PROFILE_FUNCTION();

        if (initialData != nullptr)
            ScanForTransparency(createInfo.Width, createInfo.Height, createInfo.Channels, initialData);
        CreateTexture(initialData);

        Renderer::PushStatistic("Loaded Textures", 1);
        Renderer::PushStatistic("Texture Memory", m_DataSize);
    }

    VulkanTexture::~VulkanTexture()
    {
        // Copy required variables for lambda
        auto sampler = m_Sampler;
        auto imGuiHandles = m_ImGuiHandles;
        auto layerViews = m_LayerViews;
        auto imageViews = m_ImageViews;
        auto images = m_Images;
        auto imageMemory = m_ImageMemory;
        auto imageCount = m_ImageCount;
        auto dataSize = m_DataSize;
        void* pointer = this;

        Renderer::PushJobQueue([=]()
        {
            VulkanDevice& device = VulkanContext::GetDevice();

            vkDestroySampler(device.Device(), sampler, nullptr);

            for (u32 frame = 0; frame < imageCount; frame++)
            {
                for (auto handle : imGuiHandles[frame])
                    ImGui_ImplVulkan_RemoveTexture(handle);

                for (auto view : layerViews[frame])
                    vkDestroyImageView(device.Device(), view, nullptr);

                vkDestroyImageView(device.Device(), imageViews[frame], nullptr);
                vkDestroyImage(device.Device(), images[frame], nullptr);
                vkFreeMemory(device.Device(), imageMemory[frame], nullptr);
            }

            TextureDeletedEvent event(pointer);
            VulkanContext::EmitEvent(event);

            Renderer::PushStatistic("Loaded Textures", -1);
            Renderer::PushStatistic("Texture Memory", -dataSize);
        });
    }

    void VulkanTexture::RegenerateMipMaps()
    {
        if (m_MipLevels == 1) return;

        VulkanDevice& device = VulkanContext::GetDevice();

        VulkanCommon::TransitionImageLayout(
            device.Device(),
            VulkanContext::GetGraphicsPool(),
            device.GraphicsQueue(),
            GetImage(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            m_MipLevels,
            m_Info.ArrayCount
        );
        VulkanCommon::GenerateMipmaps(
            device.Device(),
            device.PhysicalDevice(),
            VulkanContext::GetGraphicsPool(),
            device.GraphicsQueue(),
            GetImage(),
            m_Format,
            m_Info.Width, m_Info.Height,
            m_MipLevels,
            m_Info.ArrayCount
        );
    }

    void VulkanTexture::RegenerateMipMapsSync(Framebuffer* buffer)
    {
        if (m_MipLevels == 1) return;

        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanFramebuffer* framebuffer = (VulkanFramebuffer*)buffer;

        HE_ENGINE_ASSERT(framebuffer->WasBoundThisFrame(), "Passed framebuffer must have been bound this frame");
        HE_ENGINE_ASSERT(!framebuffer->WasSubmittedThisFrame(), "Passed framebuffer must not have been submitted yet this frame");

        VkCommandBuffer cmdBuffer = VulkanCommon::BeginSingleTimeCommands(device.Device(), VulkanContext::GetGraphicsPool(), true);
        VulkanCommon::TransitionImageLayout(
            device.Device(),
            cmdBuffer,
            GetImage(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            m_MipLevels,
            m_Info.ArrayCount
        );
        VulkanCommon::GenerateMipmaps(
            device.Device(),
            device.PhysicalDevice(),
            cmdBuffer,
            GetImage(),
            m_Format,
            m_Info.Width, m_Info.Height,
            m_MipLevels,
            m_Info.ArrayCount
        );
        VulkanCommon::EndSingleTimeCommands(device.Device(), nullptr, cmdBuffer, nullptr, true);
        framebuffer->PushAuxiliaryCommandBuffer(cmdBuffer);
    }

    void* VulkanTexture::GetPixelData()
    {
        HE_ENGINE_ASSERT(m_Info.AllowCPURead, "Cannot read pixel data of texture that does not have 'AllowCPURead' enabled");

        return m_CpuBuffer->GetMappedMemory();
    }

    void* VulkanTexture::GetImGuiHandle(u32 layerIndex, u32 mipLevel)
    {
        UpdateFrameIndex();
        return m_ImGuiHandles[m_InFlightFrameIndex][layerIndex * m_MipLevels + mipLevel];
    }

    void VulkanTexture::CreateTexture(void* data)
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkDeviceSize imageSize = m_Info.Width * m_Info.Height * m_Info.Channels;
        m_GeneralFormat = BufferDataTypeColorFormat(m_Info.DataType, m_Info.Channels);
        m_Format = VulkanCommon::ColorFormatToVulkan(m_GeneralFormat);
        if (m_Format == VK_FORMAT_R8G8B8A8_SRGB)
            m_Format = VK_FORMAT_R8G8B8A8_UNORM; // we want to use unorm for textures

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        m_MipLevels = m_Info.MipCount;
        u32 maxMipLevels = static_cast<u32>(floor(log2(std::max(m_Info.Width, m_Info.Height)))) + 1;
        if (m_MipLevels > maxMipLevels || m_MipLevels == 0)
            m_MipLevels = maxMipLevels;

        m_ImageCount = m_Info.UsageType == BufferUsageType::Dynamic ? Renderer::FrameBufferCount : 1;

        // if the texture is cpu visible, create the readonly buffer
        if (m_Info.AllowCPURead)
        {
            m_CpuBuffer = std::dynamic_pointer_cast<VulkanBuffer>(Buffer::Create(
                Buffer::Type::Pixel,
                BufferUsageType::Dynamic,
                { m_Info.DataType },
                m_Info.Width * m_Info.Height * m_Info.Channels,
                data
            ));
        }

        CreateSampler();

        // calculate memory footprint of the texture
        u32 componentSize = BufferDataTypeSize(m_Info.DataType);
        for (u32 i = 0; i < m_MipLevels; i++)
            m_DataSize += GetMipWidth(i) * GetMipHeight(i) * m_Info.Channels * componentSize;
        m_DataSize *= m_Info.ArrayCount * m_ImageCount;

        // create image & staging buffer and transfer the data into the first layer
        if (data != nullptr)
        {
            VulkanCommon::CreateBuffer(
                device.Device(),
                device.PhysicalDevice(),
                imageSize * componentSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer,
                stagingBufferMemory
            );
            VulkanCommon::MapAndWriteBufferMemory(
                device.Device(),
                data,
                componentSize,
                static_cast<u32>(imageSize),
                stagingBufferMemory,
                0
            );
        }
        for (u32 frame = 0; frame < m_ImageCount; frame++)
        {
            VkImage& image = m_Images[frame];
            VkDeviceMemory& imageMemory = m_ImageMemory[frame];

            VulkanCommon::CreateImage(
                device.Device(),
                device.PhysicalDevice(),
                m_Info.Width, m_Info.Height,
                m_Format,
                m_MipLevels,
                m_Info.ArrayCount,
                VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                image, imageMemory,
                VK_IMAGE_LAYOUT_UNDEFINED
            );
            VulkanCommon::TransitionImageLayout(
                device.Device(),
                VulkanContext::GetGraphicsPool(),
                device.GraphicsQueue(),
                image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                m_MipLevels,
                m_Info.ArrayCount
            );
            if (data != nullptr)
            {
                VulkanCommon::CopyBufferToImage(
                    device.Device(),
                    VulkanContext::GetGraphicsPool(),
                    device.GraphicsQueue(),
                    stagingBuffer,
                    image,
                    static_cast<u32>(m_Info.Width), static_cast<u32>(m_Info.Height)
                );
                VulkanCommon::GenerateMipmaps(
                    device.Device(),
                    device.PhysicalDevice(),
                    VulkanContext::GetGraphicsPool(),
                    device.GraphicsQueue(),
                    image,
                    m_Format,
                    m_Info.Width, m_Info.Height,
                    m_MipLevels,
                    m_Info.ArrayCount
                );
            }
            else
            {
                VulkanCommon::TransitionImageLayout(
                    device.Device(),
                    VulkanContext::GetGraphicsPool(),
                    device.GraphicsQueue(),
                    image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    m_MipLevels,
                    m_Info.ArrayCount
                );
            }

            m_ImageViews[frame] = VulkanCommon::CreateImageView(device.Device(), image, m_Format, m_MipLevels, 0, m_Info.ArrayCount, 0, VK_IMAGE_ASPECT_COLOR_BIT);
            for (u32 i = 0; i < m_Info.ArrayCount; i++)
            {
                for (u32 j = 0; j < m_MipLevels; j++)
                {
                    VkImageView layerView = VulkanCommon::CreateImageView(device.Device(), image, m_Format, 1, j, 1, i, VK_IMAGE_ASPECT_COLOR_BIT);
                    m_LayerViews[frame].AddInPlace(layerView);
                    m_ImGuiHandles[frame].AddInPlace(ImGui_ImplVulkan_AddTexture(m_Sampler, layerView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
                }
            }
        }

        // destroy the staging buffer
        if (data != nullptr)
        {
            vkDestroyBuffer(device.Device(), stagingBuffer, nullptr);
            vkFreeMemory(device.Device(), stagingBufferMemory, nullptr);
        }
    }

    void VulkanTexture::CreateSampler()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device.PhysicalDevice(), &properties);
        
        VkSamplerReductionModeCreateInfo reductionInfo{};
        reductionInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
        reductionInfo.reductionMode = VulkanCommon::SamplerReductionModeToVulkan(m_Info.SamplerState.ReductionMode);
        
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.pNext = &reductionInfo;
        samplerInfo.magFilter = VulkanCommon::SamplerFilterToVulkan(m_Info.SamplerState.MagFilter);
        samplerInfo.minFilter = VulkanCommon::SamplerFilterToVulkan(m_Info.SamplerState.MinFilter);
        samplerInfo.addressModeU = VulkanCommon::SamplerWrapModeToVulkan(m_Info.SamplerState.UVWWrap[0]);
        samplerInfo.addressModeV = VulkanCommon::SamplerWrapModeToVulkan(m_Info.SamplerState.UVWWrap[1]);
        samplerInfo.addressModeW = VulkanCommon::SamplerWrapModeToVulkan(m_Info.SamplerState.UVWWrap[2]);
        samplerInfo.anisotropyEnable = m_Info.SamplerState.AnisotropyEnable;
        samplerInfo.maxAnisotropy = std::min(static_cast<float>(m_Info.SamplerState.MaxAnisotropy), properties.limits.maxSamplerAnisotropy);
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

    void VulkanTexture::UpdateFrameIndex()
    {
        if (m_ImageCount == 1) return;

        if (App::Get().GetFrameCount() != m_LastUpdateFrame)
        {
            // it is a new frame so we need to get the new flight frame index
            VulkanContext& mainContext = static_cast<VulkanContext&>(Window::GetMainWindow().GetContext());

            m_InFlightFrameIndex = mainContext.GetSwapChain().GetInFlightFrameIndex();
            m_LastUpdateFrame = App::Get().GetFrameCount();
        }
    }
}