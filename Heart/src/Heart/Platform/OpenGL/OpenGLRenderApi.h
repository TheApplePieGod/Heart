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

        void SetLineWidth(float width) override;

        void DrawIndexed(u32 indexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount) override;
        void Draw(u32 vertexCount, u32 vertexOffset, u32 instanceCount) override;

        void DrawIndexedIndirect(Buffer* indirectBuffer, u32 commandOffset, u32 drawCount) override;

        void RenderFramebuffers(GraphicsContext& context, const std::vector<FramebufferSubmission>& submissions) override;

        void DispatchComputePipelines(GraphicsContext& context, const std::vector<ComputePipeline*>& pipelines) override {}

    private:
    };
}