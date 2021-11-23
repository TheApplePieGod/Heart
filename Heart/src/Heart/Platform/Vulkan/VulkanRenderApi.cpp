#include "htpch.h"
#include "VulkanRenderApi.h"

#include "Heart/Core/Timing.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanFramebuffer.h"
#include "Heart/Platform/Vulkan/VulkanBuffer.h"
#include "GLFW/glfw3.h"

namespace Heart
{
    VulkanRenderApi::VulkanRenderApi()
    {

    }

    VulkanRenderApi::~VulkanRenderApi()
    {

    }

    void VulkanRenderApi::SetViewport(u32 x, u32 y, u32 width, u32 height)
    {
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (f32)width;
        viewport.height = (f32)width;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(), 0, 1, &viewport);
    }

    void VulkanRenderApi::ResizeWindow(GraphicsContext& _context, u32 width, u32 height)
    {
        VulkanContext& context = static_cast<VulkanContext&>(_context);

        context.GetSwapChain().InvalidateSwapChain(width, height);
    }

    void VulkanRenderApi::BindVertexBuffer(Buffer& _buffer)
    {
        HE_PROFILE_FUNCTION();

        // To maintain consistency with opengl requirement
        HE_ENGINE_ASSERT(VulkanContext::GetBoundFramebuffer()->GetBoundPipeline() != nullptr, "Must bind graphics pipeline before calling BindVertexBuffer");

        VkBuffer buffer = static_cast<VulkanBuffer&>(_buffer).GetBuffer();

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(), 0, 1, &buffer, offsets);
    }

    void VulkanRenderApi::BindIndexBuffer(Buffer& _buffer)
    {
        HE_PROFILE_FUNCTION();

        // To maintain consistency with opengl requirement
        HE_ENGINE_ASSERT(VulkanContext::GetBoundFramebuffer()->GetBoundPipeline() != nullptr, "Must bind graphics pipeline before calling BindIndexBuffer");

        VkBuffer buffer = static_cast<VulkanBuffer&>(_buffer).GetBuffer();

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindIndexBuffer(VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(), buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanRenderApi::DrawIndexed(u32 indexCount, u32 vertexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("VulkanRenderApi::DrawIndexed");

        HE_ENGINE_ASSERT(VulkanContext::GetBoundFramebuffer()->CanDraw(), "Framebuffer is not ready to draw (did you bind all of your shader resources?)");
        vkCmdDrawIndexed(VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(), indexCount, instanceCount, indexOffset, vertexOffset, 0);
    }

    void VulkanRenderApi::Draw(u32 vertexCount, u32 vertexOffset, u32 instanceCount)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("VulkanRenderApi::Draw");

        HE_ENGINE_ASSERT(VulkanContext::GetBoundFramebuffer()->CanDraw(), "Framebuffer is not ready to draw (did you bind all of your shader resources?)");
        vkCmdDraw(VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(), vertexCount, instanceCount, vertexOffset, 0);
    }

    void VulkanRenderApi::RenderFramebuffers(GraphicsContext& _context, const std::vector<Framebuffer*>& framebuffers)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("VulkanRenderApi::RenderFramebuffers");

        VulkanContext& context = static_cast<VulkanContext&>(_context);

        std::vector<VkCommandBuffer> submittingBuffers;
        for (auto& _buffer : framebuffers)
        {
            VulkanFramebuffer* buffer = static_cast<VulkanFramebuffer*>(_buffer);
            buffer->Submit();
            submittingBuffers.emplace_back(buffer->GetCommandBuffer());
        }

        context.GetSwapChain().SubmitCommandBuffers(submittingBuffers);
    }
}