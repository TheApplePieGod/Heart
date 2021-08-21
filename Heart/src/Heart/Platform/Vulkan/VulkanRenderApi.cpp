#include "htpch.h"
#include "VulkanRenderApi.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Heart/Platform/Vulkan/VulkanIndexBuffer.h"
#include "GLFW/glfw3.h"

namespace Heart
{
    VulkanRenderApi::VulkanRenderApi()
    {
        HE_ENGINE_LOG_TRACE("Initializing VulkanRenderApi");

        
    }

    VulkanRenderApi::~VulkanRenderApi()
    {

    }

    void VulkanRenderApi::SetViewport(GraphicsContext& _context, u32 x, u32 y, u32 width, u32 height)
    {
        VulkanContext& context = static_cast<VulkanContext&>(_context);
        context.GetSwapChain().InvalidateSwapChain(width, height);
    }

    void VulkanRenderApi::BindVertexBuffer(const VertexBuffer& _buffer)
    {
        VkBuffer buffer = static_cast<const VulkanVertexBuffer&>(_buffer).GetBuffer();

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(VulkanContext::GetBoundCommandBuffer(), 0, 1, &buffer, offsets);
    }

    void VulkanRenderApi::BindIndexBuffer(const IndexBuffer& _buffer)
    {
        VkBuffer buffer = static_cast<const VulkanIndexBuffer&>(_buffer).GetBuffer();

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindIndexBuffer(VulkanContext::GetBoundCommandBuffer(), buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanRenderApi::DrawIndexed(u32 indexCount, u32 vertexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount)
    {
        vkCmdDrawIndexed(VulkanContext::GetBoundCommandBuffer(), indexCount, instanceCount, indexOffset, vertexOffset, 0);
    }
}