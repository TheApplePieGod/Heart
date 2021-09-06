#include "htpch.h"
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
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
        }

        u32 presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
        }

        return details;
    }

    VulkanCommon::QueueFamilyIndices VulkanCommon::GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        VulkanCommon::QueueFamilyIndices indices;

        u32 supportedQueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &supportedQueueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> supportedQueueFamilies(supportedQueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &supportedQueueFamilyCount, supportedQueueFamilies.data());

        int i = 0;
        for (const auto& family : supportedQueueFamilies)
        {
            if (indices.IsComplete())
                break;

            if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.GraphicsFamily = i;

            if (family.queueFlags & VK_QUEUE_COMPUTE_BIT)
                indices.ComputeFamily = i;

            if (family.queueFlags & VK_QUEUE_TRANSFER_BIT)
                indices.TransferFamily = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
                indices.PresentFamily = i;
            
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

    void VulkanCommon::CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, u32 width, u32 height, VkFormat format, u32 mipLevels, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageLayout initialLayout)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<u32>(width);
        imageInfo.extent.height = static_cast<u32>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = initialLayout;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = numSamples;
        imageInfo.flags = 0; // Optional

        HE_VULKAN_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        HE_VULKAN_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    VkImageView VulkanCommon::CreateImageView(VkDevice device, VkImage image, VkFormat format, u32 mipLevels, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView view;
        HE_VULKAN_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &view));

        return view;
    }

    VkCommandBuffer VulkanCommon::BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        HE_VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        HE_VULKAN_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        return commandBuffer;
    }

    void VulkanCommon::EndSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue submitQueue)
    {
        HE_VULKAN_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        HE_VULKAN_CHECK_RESULT(vkQueueSubmit(submitQueue, 1, &submitInfo, VK_NULL_HANDLE));

        vkQueueWaitIdle(submitQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
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

    void VulkanCommon::TransitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

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
        else
            throw std::invalid_argument("Unsupported layout transition");

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        EndSingleTimeCommands(device, commandPool, commandBuffer, transferQueue);
    }

    void VulkanCommon::CopyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue, VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
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


    VkFormat VulkanCommon::ColorFormatToVulkan(ColorFormat format)
    {
        // TODO: make these more robust
        switch (format)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified ColorFormat"); } break;
            case ColorFormat::R8: return VK_FORMAT_R8_SRGB;
            case ColorFormat::RG8: return VK_FORMAT_R8G8_SRGB;
            case ColorFormat::RGB8: return VK_FORMAT_R8G8B8_SRGB;
            case ColorFormat::RGBA8: return VK_FORMAT_R8G8B8A8_SRGB;
            case ColorFormat::RGBA32: return VK_FORMAT_R32G32B32A32_SFLOAT;
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
            case VertexTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case VertexTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
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
            case CullMode::Both: return VK_CULL_MODE_FRONT_AND_BACK;
        }

        return VK_CULL_MODE_NONE;
    }

    VkDescriptorType VulkanCommon::ShaderResourceTypeToVulkan(ShaderResourceType type)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified ShaderResourceType"); } break;
            case ShaderResourceType::Texture : return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case ShaderResourceType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case ShaderResourceType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        }

        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }

    VkShaderStageFlags VulkanCommon::ShaderResourceAccessTypeToVulkan(ShaderResourceAccessType type)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Vulkan does not support specified ShaderResourceType"); } break;
            case ShaderResourceAccessType::Vertex : return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderResourceAccessType::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
            case ShaderResourceAccessType::Both: return (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }
}