#pragma once

#include "Heart/Renderer/Buffer.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Platform/Vulkan/VulkanSwapChain.h"

namespace Heart
{    
    class VulkanBuffer : public Buffer
    {
    public:
        VulkanBuffer(Type type, BufferUsageType usage, const BufferLayout& layout, u32 elementCount, void* initialData);
        ~VulkanBuffer() override;

        void SetData(void* data, u32 elementCount, u32 elementOffset) override;

        VkBuffer GetBuffer();
        VkDeviceMemory GetMemory();
        void* GetMappedMemory();

    protected:
        void CreateBuffer(VkDeviceSize size, VkBuffer& outBuffer, VkDeviceMemory& outMemory, VkBuffer& outStagingBuffer, VkDeviceMemory& outStagingMemory);
        void UpdateFrameIndex();
        void FlushWrites(VkBuffer src, VkBuffer dst);

    protected:
        std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_Buffers;
        std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> m_BufferMemory;
        std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_StagingBuffers;
        std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> m_StagingBufferMemory;
        std::array<void*, MAX_FRAMES_IN_FLIGHT> m_MappedMemory;
        u64 m_LastUpdateFrame = 0;
        u32 m_InFlightFrameIndex = 0;
        bool m_UsesStaging = false;
    };
}