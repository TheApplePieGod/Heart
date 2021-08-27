#include "htpch.h"
#include "OpenGLRenderApi.h"

#include "glad/glad.h"
#include "Heart/Platform/OpenGL/OpenGLBuffer.h"
#include "Heart/Platform/OpenGL/OpenGLContext.h"
#include "Heart/Platform/OpenGL/OpenGLGraphicsPipeline.h"
#include "Heart/Platform/OpenGL/OpenGLCommon.h"
#include "Heart/Platform/OpenGL/OpenGLFramebuffer.h"

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
        HE_ENGINE_ASSERT(OpenGLContext::GetBoundGraphicsPipeline() != nullptr, "Must bind graphics pipeline before calling BindVertexBuffer");
        u32 bufferId = static_cast<OpenGLBuffer&>(_buffer).GetBufferId();
        glBindVertexBuffer(0, bufferId, 0, OpenGLContext::GetBoundGraphicsPipeline()->GetVertexLayoutStride());
    }

    void OpenGLRenderApi::BindIndexBuffer(Buffer& _buffer)
    {
        HE_ENGINE_ASSERT(OpenGLContext::GetBoundGraphicsPipeline() != nullptr, "Must bind graphics pipeline before calling BindIndexBuffer");
        u32 bufferId = static_cast<OpenGLBuffer&>(_buffer).GetBufferId();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferId);
    }

    void OpenGLRenderApi::DrawIndexed(u32 indexCount, u32 vertexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount)
    {
        HE_ENGINE_ASSERT(OpenGLContext::GetBoundGraphicsPipeline() != nullptr, "Must bind graphics pipeline before calling DrawIndexed");
        glDrawElementsInstancedBaseVertex(OpenGLCommon::VertexTopologyToOpenGL(OpenGLContext::GetBoundGraphicsPipeline()->GetVertexTopology()), indexCount, GL_UNSIGNED_INT, (void*)(indexOffset * sizeof(u32)), instanceCount, vertexOffset);
    }

    void OpenGLRenderApi::RenderFramebuffers(GraphicsContext& _context, const std::vector<Framebuffer*>& framebuffers)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}