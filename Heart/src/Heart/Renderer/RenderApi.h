#pragma once

#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Renderer/VertexBuffer.h"
#include "Heart/Renderer/IndexBuffer.h"

namespace Heart
{
    class RenderApi
    {
    public:
        enum class Type
        {
            None = 0, Vulkan = 1
        };

    public:
        virtual ~RenderApi() = default;

        virtual void SetViewport(GraphicsContext& context, u32 x, u32 y, u32 width, u32 height) = 0;
        virtual void BindVertexBuffer(const VertexBuffer& buffer) = 0;
        virtual void BindIndexBuffer(const IndexBuffer& buffer) = 0;
        virtual void DrawIndexed(u32 indexCount, u32 vertexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount) = 0;

    private:
        
    };
}