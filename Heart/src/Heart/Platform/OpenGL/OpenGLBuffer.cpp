#include "htpch.h"
#include "OpenGLBuffer.h"

#include "glad/glad.h"

namespace Heart
{
    // TODO: some way of hinting GL_STATIC_DRAW vs GL_DYNAMIC_DRAW
    OpenGLBuffer::OpenGLBuffer(Type type, const BufferLayout& layout, u32 elementCount, void* initialData)
        : Buffer(type, layout, elementCount)
    {
        glGenBuffers(1, &m_BufferId);

        switch (m_Type)
        {
            default: { HE_ENGINE_ASSERT("Failed to create OpenGLBuffer of unsupported type") } break;
            case Type::Uniform:
            {} break;
            case Type::Storage:
            {} break;
            case Type::Vertex:
            { glBindBuffer(GL_ARRAY_BUFFER, m_BufferId); glBufferData(GL_ARRAY_BUFFER, layout.GetStride() * elementCount, initialData, GL_STATIC_DRAW); } break;
            case Type::Index:
            { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BufferId); glBufferData(GL_ELEMENT_ARRAY_BUFFER, layout.GetStride() * elementCount, initialData, GL_STATIC_DRAW); } break;
        }
    }

    OpenGLBuffer::~OpenGLBuffer()
    {
        glDeleteBuffers(1, &m_BufferId);
    }

    void OpenGLBuffer::SetData(void* data, u32 elementCount, u32 elementOffset)
    {
        //glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        
        // glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    }
}