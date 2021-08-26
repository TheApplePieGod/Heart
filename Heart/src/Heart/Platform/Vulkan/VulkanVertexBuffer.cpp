#include "htpch.h"
#include "VulkanVertexBuffer.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"

namespace Heart
{
    VulkanVertexBuffer::VulkanVertexBuffer(const BufferLayout& layout, u32 elementCount, void* initialData)
        : VertexBuffer(layout, elementCount)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VkDeviceSize bufferSize = layout.GetStride() * elementCount;
        VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_BufferMemory);

        vkMapMemory(device.Device(), m_BufferMemory, 0, bufferSize, 0, &m_MappedMemory);
        
        if (initialData != nullptr)
            memcpy(m_MappedMemory, initialData, bufferSize);
    }

    VulkanVertexBuffer::~VulkanVertexBuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext::Sync();

        vkUnmapMemory(device.Device(), m_BufferMemory);

        vkDestroyBuffer(device.Device(), m_Buffer, nullptr);
        vkFreeMemory(device.Device(), m_BufferMemory, nullptr);
    }

    void VulkanVertexBuffer::SetData(void* data, u32 elementCount, u32 elementOffset)
    {
        // TODO: dynamic resizing
        HE_ENGINE_ASSERT(elementCount <= m_AllocatedCount, "Attempting to set data on vertex buffer which is larger than allocated size");

        memcpy(m_MappedMemory, data, m_Layout.GetStride() * elementCount);
    }
}