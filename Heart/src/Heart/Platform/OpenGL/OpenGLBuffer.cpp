#include "htpch.h"
#include "OpenGLBuffer.h"

#include "glad/glad.h"
#include "Heart/Platform/OpenGL/OpenGLCommon.h"

namespace Heart
{
    // TODO: some way of hinting GL_STATIC_DRAW vs GL_DYNAMIC_DRAW
    OpenGLBuffer::OpenGLBuffer(Type type, const BufferLayout& layout, u32 elementCount, void* initialData)
        : Buffer(type, layout, elementCount)
    {
        glGenBuffers(1, &m_BufferId);

        GLenum drawType = GL_DYNAMIC_DRAW;
        if (m_Type == Type::Vertex || m_Type == Type::Index)
            drawType = GL_STATIC_DRAW;

        glBindBuffer(OpenGLCommon::BufferTypeToOpenGL(type), m_BufferId);
        glBufferData(OpenGLCommon::BufferTypeToOpenGL(type), layout.GetStride() * elementCount, initialData, drawType);
    }

    OpenGLBuffer::~OpenGLBuffer()
    {
        glDeleteBuffers(1, &m_BufferId);
    }

    void OpenGLBuffer::SetData(void* data, u32 elementCount, u32 elementOffset)
    {
        glBindBuffer(OpenGLCommon::BufferTypeToOpenGL(m_Type), m_BufferId);
        glBufferSubData(OpenGLCommon::BufferTypeToOpenGL(m_Type), m_Layout.GetStride() * elementOffset, m_Layout.GetStride() * elementCount, data);
    }
}