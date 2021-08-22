#pragma once

#include "Heart/Renderer/Buffer.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Platform/Vulkan/VulkanSwapChain.h"

namespace Heart
{
    class VulkanBuffer : public Buffer
    {
    public:
        VulkanBuffer(const BufferLayout& layout, u32 elementCount, void* initialData);
        ~VulkanBuffer() override;

        void SetData(void* data, u32 elementCount, u32 elementOffset) override;

        inline VkBuffer GetBuffer() { UpdateFrameIndex(); return m_Buffers[m_InFlightFrameIndex]; };

    protected:
        virtual void CreateBuffer(VkDeviceSize size, VkBuffer& outBuffer, VkDeviceMemory& outMemory);
        void UpdateFrameIndex();

    protected:
        std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_Buffers;
        std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> m_BufferMemory;
        std::array<void*, MAX_FRAMES_IN_FLIGHT> m_MappedMemory;
        u64 m_LastUpdateFrame = 0;
        u32 m_InFlightFrameIndex = 0;
    };

    class VulkanBigBuffer : public VulkanBuffer
    {
    public:
        VulkanBigBuffer(const BufferLayout& layout, u32 elementCount, void* initialData)
            : VulkanBuffer(layout, elementCount, initialData)
        {}
        ~VulkanBigBuffer() = default;

    protected:
        void CreateBuffer(VkDeviceSize size, VkBuffer& outBuffer, VkDeviceMemory& outMemory) override;
    };
}