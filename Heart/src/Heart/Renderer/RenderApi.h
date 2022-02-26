#pragma once

namespace Heart
{
    class Buffer;
    class Framebuffer;
    class ComputePipeline;
    class GraphicsContext;

    struct FramebufferSubmission
    {
        Framebuffer* Framebuffer;
        ComputePipeline* PostRenderComputePipeline = nullptr;
    };

    class RenderApi
    {
    public:
        enum class Type
        {
            None = 0, Vulkan = 1, OpenGL = 2
        };
        inline static const char* TypeStrings[] = {
            "None", "Vulkan", "OpenGL"
        };

    public:
        virtual ~RenderApi() = default;

        virtual void SetViewport(u32 x, u32 y, u32 width, u32 height) = 0;
        virtual void ResizeWindow(GraphicsContext& context, u32 width, u32 height) = 0;

        virtual void BindVertexBuffer(Buffer& buffer) = 0;
        virtual void BindIndexBuffer(Buffer& buffer) = 0;

        virtual void SetLineWidth(float width) = 0;

        // all shader resources must be bound before drawing
        virtual void DrawIndexed(u32 indexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount) = 0;
        virtual void Draw(u32 vertexCount, u32 vertexOffset, u32 instanceCount) = 0;

        virtual void DrawIndexedIndirect(Buffer* indirectBuffer, u32 commandOffset, u32 drawCount) = 0;

        virtual void RenderFramebuffers(GraphicsContext& context, const std::vector<FramebufferSubmission>& submissions) = 0;

        virtual void DispatchComputePipelines(GraphicsContext& context, const std::vector<ComputePipeline*>& pipelines) = 0;

    private:
        
    };
}