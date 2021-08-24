#include "htpch.h"
#include "VulkanBuffer.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"

namespace Heart
{
    VulkanBuffer::VulkanBuffer(const BufferLayout& layout, u32 elementCount, void* initialData, Type type)
        : Buffer(layout, elementCount)
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkDeviceSize bufferSize = layout.GetStride() * elementCount;

        for (size_t i = 0; i < m_Buffers.size(); i++)
        {
            CreateBuffer(bufferSize, m_Buffers[i], m_BufferMemory[i], type);
            vkMapMemory(device.Device(), m_BufferMemory[i], 0, bufferSize, 0, &m_MappedMemory[i]);

            if (initialData != nullptr)
                memcpy(m_MappedMemory[i], initialData, bufferSize);
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext::Sync();

        for (size_t i = 0; i < m_Buffers.size(); i++)
        {
            vkUnmapMemory(device.Device(), m_BufferMemory[i]);

            vkDestroyBuffer(device.Device(), m_Buffers[i], nullptr);
            vkFreeMemory(device.Device(), m_BufferMemory[i], nullptr);
        }
    }

    void VulkanBuffer::CreateBuffer(VkDeviceSize size, VkBuffer& outBuffer, VkDeviceMemory& outMemory, Type type)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        switch (type)
        {
            default: { HE_ENGINE_ASSERT("Failed to create VulkanBuffer of unsupported type") } break;
            case Type::Uniform:
            { VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outBuffer, outMemory); } break;
            case Type::Storage:
            { VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outBuffer, outMemory); } break;
        }
    }

    void VulkanBuffer::SetData(void* data, u32 elementCount, u32 elementOffset)
    {
        UpdateFrameIndex();

        // TODO: dynamic resizing
        HE_ENGINE_ASSERT(elementCount <= m_AllocatedCount, "Attempting to set data on buffer which is larger than allocated size");

        memcpy(m_MappedMemory[m_InFlightFrameIndex], data, m_Layout.GetStride() * elementCount);
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