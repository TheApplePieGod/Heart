#include "htpch.h"
#include "VulkanBuffer.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"

namespace Heart
{
    VulkanBuffer::VulkanBuffer(Type type, const BufferLayout& layout, u32 elementCount, void* initialData)
        : Buffer(type, layout, elementCount)
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkDeviceSize bufferSize = layout.GetStride() * elementCount;

        // we only need one buffer when dealing with vertex and index buffers
        size_t bufferCount = (type == Type::Vertex || type == Type::Index) ? 1 : m_Buffers.size();
        for (size_t i = 0; i < bufferCount; i++)
        {
            CreateBuffer(bufferSize, m_Buffers[i], m_BufferMemory[i]);
            vkMapMemory(device.Device(), m_BufferMemory[i], 0, bufferSize, 0, &m_MappedMemory[i]);

            if (initialData != nullptr)
                memcpy(m_MappedMemory[i], initialData, bufferSize);
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext::Sync();

        size_t bufferCount = (m_Type == Type::Vertex || m_Type == Type::Index) ? 1 : m_Buffers.size();
        for (size_t i = 0; i < bufferCount; i++)
        {
            vkUnmapMemory(device.Device(), m_BufferMemory[i]);

            vkDestroyBuffer(device.Device(), m_Buffers[i], nullptr);
            vkFreeMemory(device.Device(), m_BufferMemory[i], nullptr);
        }
    }

    void VulkanBuffer::CreateBuffer(VkDeviceSize size, VkBuffer& outBuffer, VkDeviceMemory& outMemory)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        switch (m_Type)
        {
            default: { HE_ENGINE_ASSERT("Failed to create VulkanBuffer of unsupported type") } break;
            case Type::Uniform:
            { VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outBuffer, outMemory); } break;
            case Type::Storage:
            { VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outBuffer, outMemory); } break;
            case Type::Vertex:
            { VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outBuffer, outMemory); } break;
            case Type::Index:
            { VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outBuffer, outMemory); } break;
        }
    }

    void VulkanBuffer::SetData(void* data, u32 elementCount, u32 elementOffset)
    {
        HE_PROFILE_FUNCTION();
        
        // TODO: dynamic resizing
        HE_ENGINE_ASSERT(elementCount <= m_AllocatedCount, "Attempting to set data on buffer which is larger than allocated size");

        u32 accessingIndex = 0;
        if (m_Type != Type::Vertex && m_Type != Type::Index)
        {
            UpdateFrameIndex();
            accessingIndex = m_InFlightFrameIndex;
        }

        memcpy((char*)m_MappedMemory[accessingIndex] + m_Layout.GetStride() * elementOffset, data, m_Layout.GetStride() * elementCount);
    }

    VkBuffer VulkanBuffer::GetBuffer()
    {
        u32 accessingIndex = 0;
        if (m_Type != Type::Vertex && m_Type != Type::Index)
        {
            UpdateFrameIndex();
            accessingIndex = m_InFlightFrameIndex;
        }
        
        return m_Buffers[accessingIndex];
    }

    void VulkanBuffer::UpdateFrameIndex()
    {
        if (App::Get().GetFrameCount() != m_LastUpdateFrame)
        {
            // it is a new frame so we need to get the new flight frame index
            VulkanContext& mainContext = static_cast<VulkanContext&>(Window::GetMainWindow().GetContext());

            m_InFlightFrameIndex = mainContext.GetSwapChain().GetInFlightFrameIndex();
            m_LastUpdateFrame = App::Get().GetFrameCount();
        }
    }
}