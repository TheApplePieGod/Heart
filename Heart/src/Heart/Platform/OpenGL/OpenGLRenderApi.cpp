#include "htpch.h"
#include "OpenGLRenderApi.h"

#include <glad/glad.h>

namespace Heart
{
    OpenGLRenderApi::OpenGLRenderApi()
    {

    }

    OpenGLRenderApi::~OpenGLRenderApi()
    {

    }

    void OpenGLRenderApi::SetViewport(u32 x, u32 y, u32 width, u32 height)
    {
        glViewport(x, y, width, height);
    }

    void OpenGLRenderApi::ResizeWindow(GraphicsContext& _context, u32 width, u32 height)
    {
        glViewport(0, 0, width, height);
    }

    void OpenGLRenderApi::BindVertexBuffer(Buffer& _buffer)
    {

    }

    void OpenGLRenderApi::BindIndexBuffer(Buffer& _buffer)
    {

    }

    void OpenGLRenderApi::DrawIndexed(u32 indexCount, u32 vertexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount)
    {
        
    }

    void OpenGLRenderApi::RenderFramebuffers(GraphicsContext& _context, const std::vector<Framebuffer*>& framebuffers)
    {

    }
}