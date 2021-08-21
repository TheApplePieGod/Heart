#pragma once

#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Renderer/IndexBuffer.h"

namespace Heart
{
    class VulkanIndexBuffer : public IndexBuffer
    {
    public:
        VulkanIndexBuffer(u32 indexCount, u32* initialData);
        ~VulkanIndexBuffer() override;

        void SetData(void* data, u32 indexCount, u32 indexOffset) override;

        inline VkBuffer GetBuffer() const { return m_Buffer; }

    private:
        VkBuffer m_Buffer;
        VkDeviceMemory m_BufferMemory;
        void* m_MappedMemory;
    };
}