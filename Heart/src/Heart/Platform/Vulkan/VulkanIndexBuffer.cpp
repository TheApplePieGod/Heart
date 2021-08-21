#include "htpch.h"
#include "VulkanIndexBuffer.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"

namespace Heart
{
    VulkanIndexBuffer::VulkanIndexBuffer(u32 indexCount, void* initialData)
        : IndexBuffer(indexCount)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VkDeviceSize bufferSize = m_Layout.GetStride() * indexCount;
        VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_BufferMemory);

        vkMapMemory(device.Device(), m_BufferMemory, 0, bufferSize, 0, &m_MappedMemory);
        
        if (initialData != nullptr)
            memcpy(m_MappedMemory, initialData, bufferSize);
    }

    VulkanIndexBuffer::~VulkanIndexBuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkUnmapMemory(device.Device(), m_BufferMemory);

        vkDestroyBuffer(device.Device(), m_Buffer, nullptr);
        vkFreeMemory(device.Device(), m_BufferMemory, nullptr);
    }

    void VulkanIndexBuffer::SetData(void* data, u32 indexCount, u32 indexOffset)
    {
        // TODO: dynamic resizing
        HE_ENGINE_ASSERT(indexCount <= m_AllocatedCount, "Attempting to set data on vertex buffer which is larger than allocated size");

        memcpy(m_MappedMemory, data, m_Layout.GetStride() * indexCount);
    }
}