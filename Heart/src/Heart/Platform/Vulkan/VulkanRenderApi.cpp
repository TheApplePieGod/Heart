#include "hepch.h"
#include "VulkanRenderApi.h"

#include "Heart/Core/Timing.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanFramebuffer.h"
#include "Heart/Platform/Vulkan/VulkanBuffer.h"
#include "Heart/Platform/Vulkan/VulkanComputePipeline.h"
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

    void VulkanRenderApi::SetLineWidth(float width)
    {
        HE_ENGINE_ASSERT(width > 0.f && width <= 10.f, "Line width must be > 0 and <= 10"); // opengl constraint

        vkCmdSetLineWidth(VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(), width);
    }

    void VulkanRenderApi::DrawIndexed(u32 indexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("VulkanRenderApi::DrawIndexed");

        HE_ENGINE_ASSERT(VulkanContext::GetBoundFramebuffer()->CanDraw(), "Framebuffer is not ready to draw (did you bind & flush all of your shader resources?)");
        vkCmdDrawIndexed(VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(), indexCount, instanceCount, indexOffset, vertexOffset, 0);
    }

    void VulkanRenderApi::Draw(u32 vertexCount, u32 vertexOffset, u32 instanceCount)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("VulkanRenderApi::Draw");

        HE_ENGINE_ASSERT(VulkanContext::GetBoundFramebuffer()->CanDraw(), "Framebuffer is not ready to draw (did you bind & flush all of your shader resources?)");
        vkCmdDraw(VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(), vertexCount, instanceCount, vertexOffset, 0);
    }

    void VulkanRenderApi::DrawIndexedIndirect(Buffer* indirectBuffer, u32 commandOffset, u32 drawCount)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("VulkanRenderApi::DrawIndexedIndirect");
        
        // TODO: use vkCmdDrawIndexedIndirectCount
        HE_ENGINE_ASSERT(VulkanContext::GetBoundFramebuffer()->CanDraw(), "Framebuffer is not ready to draw (did you bind & flush all of your shader resources?)");
        vkCmdDrawIndexedIndirect(
            VulkanContext::GetBoundFramebuffer()->GetCommandBuffer(),
            ((VulkanBuffer*)indirectBuffer)->GetBuffer(),
            commandOffset * indirectBuffer->GetLayout().GetStride(),
            drawCount,
            indirectBuffer->GetLayout().GetStride()
        );
    }

    void VulkanRenderApi::RenderFramebuffers(GraphicsContext& _context, const std::vector<FramebufferSubmission>& submissions)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("VulkanRenderApi::RenderFramebuffers");

        VulkanContext& context = static_cast<VulkanContext&>(_context);

        std::vector<VulkanFramebufferSubmit> submits;
        for (auto& submission : submissions)
        {
            HE_ENGINE_ASSERT(submission.Framebuffer, "Cannot submit a nullptr framebuffer");

            VulkanComputePipeline* preComp = submission.PreRenderComputePipeline ? static_cast<VulkanComputePipeline*>(submission.PreRenderComputePipeline) : nullptr;
            VulkanComputePipeline* postComp = submission.PostRenderComputePipeline ? static_cast<VulkanComputePipeline*>(submission.PostRenderComputePipeline) : nullptr;
            VulkanFramebuffer* buffer = static_cast<VulkanFramebuffer*>(submission.Framebuffer);
            buffer->Submit();
            if (preComp) preComp->Submit();
            if (postComp) postComp->Submit();

            submits.push_back({
                buffer->GetCommandBuffer(),
                buffer->GetTransferCommandBuffer(),
                preComp ? preComp->GetCommandBuffer() : nullptr,
                postComp ? postComp->GetCommandBuffer() : nullptr
            });
        }

        context.GetSwapChain().SubmitCommandBuffers(submits);
    }

    void VulkanRenderApi::DispatchComputePipelines(GraphicsContext& _context, const std::vector<ComputePipeline*>& pipelines)
    {
        // HE_PROFILE_FUNCTION();
        // auto timer = AggregateTimer("VulkanRenderApi::DispatchComputePipelines");

        // VulkanContext& context = static_cast<VulkanContext&>(_context);

        // std::vector<VulkanFramebufferSubmit> submits;
        // for (auto& _pipeline : pipelines)
        // {
        //     VulkanComputePipeline* pipeline = static_cast<VulkanComputePipeline*>(_pipeline);
        //     pipeline->Submit();
        //     submits.push_back({ buffer->GetCommandBuffer(), buffer->GetTransferCommandBuffer() });
        // }

        // context.GetSwapChain().SubmitCommandBuffers(submits);
    }
}