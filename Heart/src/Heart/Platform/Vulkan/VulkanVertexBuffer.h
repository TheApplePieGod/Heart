#pragma once

#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Renderer/VertexBuffer.h"

namespace Heart
{
    class VulkanVertexBuffer : public VertexBuffer
    {
    public:
        VulkanVertexBuffer(const BufferLayout& layout, u32 elementCount, void* initialData);
        ~VulkanVertexBuffer() override;

        void SetData(void* data, u32 elementCount, u32 elementOffset) override;

        inline VkBuffer GetBuffer() const { return m_Buffer; }

    private:
        VkBuffer m_Buffer;
        VkDeviceMemory m_BufferMemory;
        void* m_MappedMemory;
    };
}