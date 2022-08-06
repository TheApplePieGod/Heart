#include "hepch.h"
#include "VulkanCommon.h"

namespace Heart
{
    VulkanCommon::SwapChainSupportDetails VulkanCommon::GetSwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);
        
        u32 formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.Formats.Resize(formatCount, false);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.Data());
        }

        u32 presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.PresentModes.Resize(presentModeCount, false);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.Data());
        }

        return details;
    }

    VulkanCommon::QueueFamilyIndices VulkanCommon::GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        VulkanCommon::QueueFamilyIndices indices;

        u32 supportedQueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &supportedQueueFamilyCount, nullptr);
        HVector<VkQueueFamilyProperties> supportedQueueFamilies(supportedQueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &supportedQueueFamilyCount, supportedQueueFamilies.Data());

        // pass over the supported families and populate indices with the first available option
        int i = 0;
        for (const auto& family : supportedQueueFamilies)
        {
            if (indices.IsComplete()) break;

            if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.GraphicsFamily = i;
                indices.GraphicsQueueCount = family.queueCount;
            }

            if (family.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                indices.ComputeFamily = i;
                indices.ComputeQueueCount = family.queueCount;
            }

            if (family.queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                indices.TransferFamily = i;
                indices.TransferQueueCount = family.queueCount;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                indices.PresentFamily = i;
                indices.PresentQueueCount = family.queueCount;
            }
            
            i++;
        }

        i = 0;
        for (const auto& family : supportedQueueFamilies)
        {
            // prioritize families that support compute but not graphics
            if (family.queueFlags & VK_QUEUE_COMPUTE_BIT && !(family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                indices.ComputeFamily = i;
                indices.ComputeQueueCount = family.queueCount;
            }

            // prioritize families that support transfer only
            if (family.queueFlags & VK_QUEUE_TRANSFER_BIT && !(family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(family.queueFlags & VK_QUEUE_COMPUTE_BIT))
            {
                indices.TransferFamily = i;
                indices.TransferQueueCount = family.queueCount;
            }

            i++;
        }

        return indices;
    }

    u32 VulkanCommon::FindMemoryType(VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (u32 i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        HE_ENGINE_ASSERT(false, "Failed to find suitable memory type");
        return 0;
    }

    void VulkanCommon::CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, u32 width, u32 height, VkFormat format, u32 mipLevels, u32 layerCount, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageLayout initialLayout)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<u32>(width);
        imageInfo.extent.height = static_cast<u32>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = layerCount;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = initialLayout;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = numSamples;
        imageInfo.flags = layerCount == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

        HE_VULKAN_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        HE_VULKAN_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    VkImageView VulkanCommon::CreateImageView(VkDevice device, VkImage image, VkFormat format, u32 mipLevels, u32 baseMip, u32 layerCount, u32 baseArrayLayer, VkImageAspectFlags aspectFlags)
    {
        VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
        if (layerCount > 1)
            viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        if (layerCount == 6)
            viewType = VK_IMAGE_VIEW_TYPE_CUBE;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = viewType;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = baseMip;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
        viewInfo.subresourceRange.layerCount = layerCount;

        VkImageView view;
        HE_VULKAN_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &view));

        return view;
    }

    VkCommandBuffer VulkanCommon::BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool, bool secondary)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = secondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        HE_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

        VkCommandBufferInheritanceInfo inheritanceInfo{};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.pNext = nullptr;
        inheritanceInfo.renderPass = nullptr;
        inheritanceInfo.subpass = 0;
        inheritanceInfo.framebuffer = VK_NULL_HANDLE;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = secondary ? &inheritanceInfo : nullptr;

        HE_VULKAN_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        return commandBuffer;
    }

    void VulkanCommon::EndSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue submitQueue, bool secondary)
    {
        HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

        if (!secondary)
        {
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            HE_VULKAN_CHECK_RESULT(vkQueueSubmit(submitQueue, 1, &submitInfo, VK_NULL_HANDLE));
            vkQueueWaitIdle(submitQueue);

            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        }
    }

    VkPipelineShaderStageCreateInfo VulkanCommon::DefineShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const char* entrypoint)
    {
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = stage;
        shaderStageInfo.module = shaderModule;
        shaderStageInfo.pName = entrypoint;
        shaderStageInfo.pSpecializationInfo = nullptr;

        return shaderStageInfo;
    }

    void VulkanCommon::CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        HE_VULKAN_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
        
        HE_VULKAN_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));

        HE_VULKAN_CHECK_RESULT(vkBindBufferMemory(device, buffer, bufferMemory, 0));
    }

    void VulkanCommon::MapAndWriteBufferMemory(VkDevice device, void* data, u32 dataSize, u32 elementCount, VkDeviceMemory bufferMemory, u32 elementMemoryOffset)
    {
        void* buffer;
        vkMapMemory(device, bufferMemory, elementMemoryOffset * dataSize, dataSize * elementCount, 0, &buffer);
        memcpy(buffer, data, dataSize * elementCount);
        vkUnmapMemory(device, bufferMemory);
    }

    void VulkanCommon::TransitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels, u32 layerCount)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
        TransitionImageLayout(device, commandBuffer, image, oldLayout, newLayout, mipLevels, layerCount);
        EndSingleTimeCommands(device, commandPool, commandBuffer, transferQueue);
    }

    void VulkanCommon::TransitionImageLayout(VkDevice device, VkCommandBuffer buffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels, u32 layerCount)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;

        /*
        * Undefined → transfer destination: transfer writes that don't need to wait on anything
        * Transfer destination → shader reading: shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's where we're going to use the texture
        */
        
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
            throw std::invalid_argument("Unsupported layout transition");

        vkCmdPipelineBarrier(
            buffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    void VulkanCommon::CopyBufferToBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, u32 size)
    {
        VkBufferCopy region{};
        region.srcOffset = 0;
        region.dstOffset = 0;
        region.size = size;

        vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &region);
    }

    void VulkanCommon::CopyBufferToBuffer(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, u32 size)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
        CopyBufferToBuffer(commandBuffer, srcBuffer, dstBuffer, size);
        EndSingleTimeCommands(device, commandPool, commandBuffer, transferQueue);
    }

    void VulkanCommon::CopyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue, VkBuffer srcBuffer, VkImage dstImage, u32 width, u32 height)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        EndSingleTimeCommands(device, commandPool, commandBuffer, transferQueue);
    }

    void VulkanCommon::GenerateMipmaps(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue submitQueue, VkImage image, VkFormat imageFormat, u32 width, u32 height, u32 mipLevels, u32 layerCount)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
        GenerateMipmaps(device, physicalDevice, commandBuffer, image, imageFormat, width, height, mipLevels, layerCount);
        EndSingleTimeCommands(device, commandPool, commandBuffer, submitQueue);
    }

    void VulkanCommon::GenerateMipmaps(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandBuffer buffer, VkImage image, VkFormat imageFormat, u32 width, u32 height, u32 mipLevels, u32 layerCount)
    {
        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        // TODO: store these somewhere or create them using STB
        HE_ENGINE_ASSERT(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, "Texture image format does not support linear blitting");

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;
        barrier.subresourceRange.levelCount = 1;

        int mipWidth = static_cast<int>(width);
        int mipHeight = static_cast<int>(height);

        for (u32 i = 1; i < mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = layerCount;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = layerCount;

            vkCmdBlitImage(buffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR
            );

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    VkFormat VulkanCommon::ColorFormatToVulkan(ColorFormat format)
    {
        switch (format)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified ColorFormat"); } break;
            case ColorFormat::RGBA8: return VK_FORMAT_R8G8B8A8_SRGB;
            case ColorFormat::R16F: return VK_FORMAT_R16_SFLOAT;
            case ColorFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case ColorFormat::R32F: return VK_FORMAT_R32_SFLOAT;
            case ColorFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
        }

        return VK_FORMAT_UNDEFINED;
    }

    VkSampleCountFlagBits VulkanCommon::MsaaSampleCountToVulkan(MsaaSampleCount sampleCount)
    {
        switch (sampleCount)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified MsaaSampleCount"); } break;
            case MsaaSampleCount::None: return VK_SAMPLE_COUNT_1_BIT;
            case MsaaSampleCount::Two: return VK_SAMPLE_COUNT_2_BIT;
            case MsaaSampleCount::Four: return VK_SAMPLE_COUNT_4_BIT;
            case MsaaSampleCount::Eight: return VK_SAMPLE_COUNT_8_BIT;
            case MsaaSampleCount::Sixteen: return VK_SAMPLE_COUNT_16_BIT;
            case MsaaSampleCount::Thirtytwo: return VK_SAMPLE_COUNT_32_BIT;
            case MsaaSampleCount::Sixtyfour: return VK_SAMPLE_COUNT_64_BIT;
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    VkPrimitiveTopology VulkanCommon::VertexTopologyToVulkan(VertexTopology topology)
    {
        switch (topology)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified VertexTopology"); } break;
            case VertexTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case VertexTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case VertexTopology::TriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            case VertexTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case VertexTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case VertexTopology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        }

        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    VkFormat VulkanCommon::BufferDataTypeToVulkan(BufferDataType type)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified BufferDataType"); } break;
            case BufferDataType::Bool: return VK_FORMAT_R32_UINT;
            case BufferDataType::UInt: return VK_FORMAT_R32_UINT;
            case BufferDataType::Double: return VK_FORMAT_R64_SFLOAT;
            case BufferDataType::Int: return VK_FORMAT_R32_SINT;
            case BufferDataType::Int2: return VK_FORMAT_R32G32_SINT;
            case BufferDataType::Int3: return VK_FORMAT_R32G32B32_SINT;
            case BufferDataType::Int4: return VK_FORMAT_R32G32B32A32_SINT;
            case BufferDataType::Float: return VK_FORMAT_R32_SFLOAT;
            case BufferDataType::Float2: return VK_FORMAT_R32G32_SFLOAT;
            case BufferDataType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
            case BufferDataType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            //case BufferDataType::Mat3: return VK_FORMAT_UNDEFINED;
            //case BufferDataType::Mat4: return VK_FORMAT_UNDEFINED;
        }

        return VK_FORMAT_UNDEFINED;
    }

    VkCullModeFlagBits VulkanCommon::CullModeToVulkan(CullMode mode)
    {
        switch (mode)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified CullMode"); } break;
            case CullMode::None: return VK_CULL_MODE_NONE;
            case CullMode::Backface: return VK_CULL_MODE_BACK_BIT;
            case CullMode::Frontface: return VK_CULL_MODE_FRONT_BIT;
            case CullMode::BackAndFront: return VK_CULL_MODE_FRONT_AND_BACK;
        }

        return VK_CULL_MODE_NONE;
    }

    VkFrontFace VulkanCommon::WindingOrderToVulkan(WindingOrder order)
    {
        switch (order)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified WindingOrder"); } break;
            case WindingOrder::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
            case WindingOrder::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }

        return VK_FRONT_FACE_MAX_ENUM;
    }

    VkDescriptorType VulkanCommon::ShaderResourceTypeToVulkan(ShaderResourceType type)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified ShaderResourceType"); } break;
            case ShaderResourceType::Texture: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case ShaderResourceType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case ShaderResourceType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            case ShaderResourceType::SubpassInput: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        }

        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }

    VkShaderStageFlags VulkanCommon::ShaderResourceAccessTypeToVulkan(ShaderResourceAccessType type)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified ShaderResourceType"); } break;
            case ShaderResourceAccessType::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderResourceAccessType::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
            case ShaderResourceAccessType::Both: return (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
            case ShaderResourceAccessType::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
        }

        return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }

    VkBlendFactor VulkanCommon::BlendFactorToVulkan(BlendFactor factor)
    {
        switch (factor)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified BlendFactor"); } break;
            case BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
            case BlendFactor::One: return VK_BLEND_FACTOR_ONE;

            case BlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
            case BlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case BlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
            case BlendFactor::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;

            case BlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
            case BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case BlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
            case BlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        }

        return VK_BLEND_FACTOR_MAX_ENUM;
    }

    VkBlendOp VulkanCommon::BlendOperationToVulkan(BlendOperation op)
    {
        switch (op)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified BlendOperation"); } break;
            case BlendOperation::Add: return VK_BLEND_OP_ADD;
            case BlendOperation::Subtract: return VK_BLEND_OP_SUBTRACT;
            case BlendOperation::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
            case BlendOperation::Min: return VK_BLEND_OP_MIN;
            case BlendOperation::Max: return VK_BLEND_OP_MAX;
        }

        return VK_BLEND_OP_MAX_ENUM;
    }

    VkFilter VulkanCommon::SamplerFilterToVulkan(SamplerFilter filter)
    {
        switch (filter)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified SamplerFilter"); } break;
            case SamplerFilter::Linear: return VK_FILTER_LINEAR;
            case SamplerFilter::Nearest: return VK_FILTER_NEAREST;
        }

        return VK_FILTER_MAX_ENUM;
    }

    VkSamplerAddressMode VulkanCommon::SamplerWrapModeToVulkan(SamplerWrapMode mode)
    {
        switch (mode)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified SamplerWrapMode"); } break;
            case SamplerWrapMode::ClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case SamplerWrapMode::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case SamplerWrapMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case SamplerWrapMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        }

        return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
    }

    VkSamplerReductionMode VulkanCommon::SamplerReductionModeToVulkan(SamplerReductionMode mode)
    {
        switch (mode)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified SamplerReductionMode"); } break;
            case SamplerReductionMode::WeightedAverage: return VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
            case SamplerReductionMode::Min: return VK_SAMPLER_REDUCTION_MODE_MIN;
            case SamplerReductionMode::Max: return VK_SAMPLER_REDUCTION_MODE_MAX;
        }

        return VK_SAMPLER_REDUCTION_MODE_MAX_ENUM;
    }
}