#pragma once

#include "Heart/Renderer/RenderApi.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanRenderApi : public RenderApi
    {
    public:
        VulkanRenderApi();
        ~VulkanRenderApi() override;

        void SetViewport(u32 x, u32 y, u32 width, u32 height) override;
        void ResizeWindow(GraphicsContext& context, u32 width, u32 height) override;

        void BindVertexBuffer(const VertexBuffer& buffer) override;
        void BindIndexBuffer(const IndexBuffer& buffer) override;

        void DrawIndexed(u32 indexCount, u32 vertexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount) override;

        void RenderFramebuffers(GraphicsContext& context, const std::vector<Framebuffer*>& framebuffers) override;

    private:
    };
}