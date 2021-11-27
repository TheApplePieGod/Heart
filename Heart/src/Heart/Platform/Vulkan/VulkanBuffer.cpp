#include "htpch.h"
#include "VulkanBuffer.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"

namespace Heart
{
    VulkanBuffer::VulkanBuffer(Type type, BufferUsageType usage, const BufferLayout& layout, u32 elementCount, void* initialData)
        : Buffer(type, usage, layout, elementCount)
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VkDeviceSize bufferSize = layout.GetStride() * elementCount;

        switch (type)
        {
            default: break;
            case Type::Uniform:
            {
                if (elementCount > 1)
                {
                    VkDeviceSize minAlignment = device.PhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
                    if (layout.GetStride() % minAlignment != 0)
                    {
                        HE_ENGINE_LOG_CRITICAL("Uniform buffer layout must be a multiple of {0} but is {1}", minAlignment, layout.GetStride());
                        HE_ENGINE_ASSERT(false);
                    }
                }
            }
            case Type::Storage:
            {
                if (elementCount > 1)
                {
                    VkDeviceSize minAlignment = device.PhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;
                    if (layout.GetStride() % minAlignment != 0)
                    {
                        HE_ENGINE_LOG_CRITICAL("Storage buffer layout must be a multiple of {0} but is {1}", minAlignment, layout.GetStride());
                        HE_ENGINE_ASSERT(false);
                    }
                }
            }
            case Type::Pixel:
            { m_UsesStaging = false; } break;
            case Type::Indirect:
            { m_UsesStaging = false; } break;
            case Type::Vertex:
            case Type::Index:
            {
                m_UsesStaging = true;
                m_StagingDirection = StagingDirection::CPUToGPU;
            } break;
        }

        HE_ENGINE_ASSERT(usage == BufferUsageType::Static || usage == BufferUsageType::Dynamic, "Vulkan does not support specified BufferUsageType");

        // we only need one buffer when dealing with static data
        size_t bufferCount = usage == BufferUsageType::Static ? 1 : m_Buffers.size();
        for (size_t i = 0; i < bufferCount; i++)
        {
            CreateBuffer(bufferSize, m_Buffers[i], m_BufferMemory[i], m_StagingBuffers[i], m_StagingBufferMemory[i]);

            if (m_UsesStaging && m_StagingDirection == StagingDirection::CPUToGPU)
                vkMapMemory(device.Device(), m_StagingBufferMemory[i], 0, bufferSize, 0, &m_MappedMemory[i]);
            else
                vkMapMemory(device.Device(), m_BufferMemory[i], 0, bufferSize, 0, &m_MappedMemory[i]);

            if (initialData != nullptr)
                memcpy(m_MappedMemory[i], initialData, bufferSize);
            else if (m_Usage == BufferUsageType::Static)
            { HE_ENGINE_LOG_WARN("Creating a static buffer with no initial data"); }

            if (m_UsesStaging)
            {
                // perform the initial transfer
                if (initialData != nullptr)
                    FlushWrites(m_StagingBuffers[i], m_Buffers[i]);

                // if static, we are not performing any more writes, so delete the staging buffer
                if (usage == BufferUsageType::Static)
                {
                    vkUnmapMemory(device.Device(), m_StagingBufferMemory[i]);
                    vkDestroyBuffer(device.Device(), m_StagingBuffers[i], nullptr);
                    vkFreeMemory(device.Device(), m_StagingBufferMemory[i], nullptr);
                }
            }
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        VulkanDevice& device = VulkanContext::GetDevice();
        VulkanContext::Sync();

        size_t bufferCount = m_Usage == BufferUsageType::Static ? 1 : m_Buffers.size();
        for (size_t i = 0; i < bufferCount; i++)
        {
            if (m_UsesStaging && m_StagingDirection == StagingDirection::CPUToGPU)
            {
                if (m_Usage == BufferUsageType::Dynamic)
                    vkUnmapMemory(device.Device(), m_StagingBufferMemory[i]);
            }
            else
                vkUnmapMemory(device.Device(), m_BufferMemory[i]);
                
            if (m_UsesStaging && m_Usage == BufferUsageType::Dynamic) // statc should already have the staging buffer destroyed
            {
                vkDestroyBuffer(device.Device(), m_StagingBuffers[i], nullptr);
                vkFreeMemory(device.Device(), m_StagingBufferMemory[i], nullptr);
            }     
            
            vkDestroyBuffer(device.Device(), m_Buffers[i], nullptr);
            vkFreeMemory(device.Device(), m_BufferMemory[i], nullptr);  
        }
    }

    void VulkanBuffer::CreateBuffer(VkDeviceSize size, VkBuffer& outBuffer, VkDeviceMemory& outMemory, VkBuffer& outStagingBuffer, VkDeviceMemory& outStagingMemory)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        switch (m_Type)
        {
            default: { HE_ENGINE_ASSERT("Failed to create VulkanBuffer of unsupported type") } break;
            case Type::Uniform:
            { VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outBuffer, outMemory); } break;
            case Type::Storage:
            { VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outBuffer, outMemory); } break;
            case Type::Pixel:
            { VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outBuffer, outMemory); } break;
            case Type::Indirect:
            { VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outBuffer, outMemory); } break;
            case Type::Vertex:
            {
                VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outStagingBuffer, outStagingMemory);
                VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory);
            } break;
            case Type::Index:
            {
                VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outStagingBuffer, outStagingMemory);
                VulkanCommon::CreateBuffer(device.Device(), device.PhysicalDevice(), size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory);
            } break;
        }
    }

    u32 VulkanBuffer::GetAccessingIndex()
    {
        u32 accessingIndex = 0;
        if (m_Usage == BufferUsageType::Dynamic)
        {
            UpdateFrameIndex();
            accessingIndex = m_InFlightFrameIndex;
        }
        return accessingIndex;
    }

    void VulkanBuffer::SetData(void* data, u32 elementCount, u32 elementOffset)
    {
        HE_PROFILE_FUNCTION();
        
        // TODO: dynamic resizing
        HE_ENGINE_ASSERT(elementCount + elementOffset <= m_AllocatedCount, "Attempting to set data on buffer which is larger than allocated size");
        if (m_Usage == BufferUsageType::Static)
        {
            HE_ENGINE_LOG_WARN("Attemting to update buffer that is marked as static");
            return;
        }
        if (m_StagingDirection == StagingDirection::GPUToCPU)
        {
            HE_ENGINE_LOG_WARN("Attemting to update buffer that retrieves data from the GPU");
            return;
        }

        u32 accessingIndex = GetAccessingIndex();

        memcpy((char*)m_MappedMemory[accessingIndex] + m_Layout.GetStride() * elementOffset, data, m_Layout.GetStride() * elementCount);

        if (m_UsesStaging)
            FlushWrites(m_StagingBuffers[accessingIndex], m_Buffers[accessingIndex]);
    }

    VkBuffer VulkanBuffer::GetBuffer()
    {
        u32 accessingIndex = GetAccessingIndex();

        return m_Buffers[accessingIndex];
    }

    VkBuffer VulkanBuffer::GetStagingBuffer()
    {
        u32 accessingIndex = GetAccessingIndex();

        return m_StagingBuffers[accessingIndex];
    }

    VkDeviceMemory VulkanBuffer::GetMemory()
    {
        u32 accessingIndex = GetAccessingIndex();

        return m_BufferMemory[accessingIndex];
    }

    VkDeviceMemory VulkanBuffer::GetStagingMemory()
    {
        u32 accessingIndex = GetAccessingIndex();

        return m_StagingBufferMemory[accessingIndex];
    }

    void* VulkanBuffer::GetMappedMemory()
    {
        u32 accessingIndex = GetAccessingIndex();

        return m_MappedMemory[accessingIndex];
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

    // TODO: should probably have an option which makes sure this is done at the end of this frame (maybe?)
    void VulkanBuffer::FlushWrites(VkBuffer src, VkBuffer dst)
    {
        HE_PROFILE_FUNCTION();

        VulkanDevice& device = VulkanContext::GetDevice();

        // copy from staging to device
        auto cmd = VulkanCommon::BeginSingleTimeCommands(device.Device(), VulkanContext::GetTransferPool());

        VkBufferCopy copy{};
        copy.srcOffset = 0;
        copy.dstOffset = 0;
        copy.size = GetAllocatedSize();

        vkCmdCopyBuffer(cmd, src, dst, 1, &copy);
        VulkanCommon::EndSingleTimeCommands(device.Device(), VulkanContext::GetTransferPool(), cmd, device.TransferQueue());
    }
}