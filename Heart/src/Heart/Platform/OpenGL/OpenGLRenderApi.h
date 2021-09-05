#pragma once

#include "Heart/Renderer/RenderApi.h"

namespace Heart
{
    class OpenGLRenderApi : public RenderApi
    {
    public:
        OpenGLRenderApi();
        ~OpenGLRenderApi() override;

        void SetViewport(u32 x, u32 y, u32 width, u32 height) override;
        void ResizeWindow(GraphicsContext& context, u32 width, u32 height) override;

        void BindVertexBuffer(Buffer& buffer) override;
        void BindIndexBuffer(Buffer& buffer) override;

        void DrawIndexed(u32 indexCount, u32 vertexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount) override;

        void RenderFramebuffers(GraphicsContext& context, const std::vector<Framebuffer*>& framebuffers) override;

    private:
    };
}