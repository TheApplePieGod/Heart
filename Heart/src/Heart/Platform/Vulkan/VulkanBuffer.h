#pragma once

#include "Heart/Renderer/Buffer.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{    
    class VulkanBuffer : public Buffer
    {
    public:
        VulkanBuffer(Type type, BufferUsageType usage, const BufferLayout& layout, u32 elementCount, void* initialData);
        ~VulkanBuffer() override;

        void SetData(void* data, u32 elementCount, u32 elementOffset) override;

        VkBuffer GetBuffer();
        VkBuffer GetStagingBuffer();
        VkDeviceMemory GetMemory();
        VkDeviceMemory GetStagingMemory();
        void* GetMappedMemory();

    private:
        enum class StagingDirection
        {
            CPUToGPU = 0,
            GPUToCPU
        };

    private:
        void CreateBuffer(VkDeviceSize size, VkBuffer& outBuffer, VkDeviceMemory& outMemory, VkBuffer& outStagingBuffer, VkDeviceMemory& outStagingMemory);
        void UpdateFrameIndex();
        void FlushWrites(VkBuffer src, VkBuffer dst);
        u32 GetAccessingIndex();

    private:
        std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_Buffers;
        std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> m_BufferMemory;
        std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_StagingBuffers;
        std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> m_StagingBufferMemory;
        std::array<void*, MAX_FRAMES_IN_FLIGHT> m_MappedMemory;
        u64 m_LastUpdateFrame = 0;
        u32 m_InFlightFrameIndex = 0;
        bool m_UsesStaging = false;
        StagingDirection m_StagingDirection = StagingDirection::CPUToGPU;
    };
}