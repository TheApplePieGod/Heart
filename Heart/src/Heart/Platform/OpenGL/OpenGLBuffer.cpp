#include "hepch.h"
#include "OpenGLBuffer.h"

#include "glad/glad.h"
#include "Heart/Platform/OpenGL/OpenGLCommon.h"

namespace Heart
{
    OpenGLBuffer::OpenGLBuffer(Type type, BufferUsageType usage, const BufferLayout& layout, u32 elementCount, void* initialData)
        : Buffer(type, usage, layout, elementCount)
    {
        glGenBuffers(1, &m_BufferId);

        if (elementCount > 1)
        {
            if (type == Type::Uniform)
            {
                int minAlignment = 0;
                glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minAlignment);
                if (layout.GetStride() % minAlignment != 0)
                {
                    HE_ENGINE_LOG_CRITICAL("Uniform buffer layout must be a multiple of {0} but is {1}", minAlignment, layout.GetStride());
                    HE_ENGINE_ASSERT(false);
                }
            }
            else if (type == Type::Storage)
            {
                int minAlignment = 0;
                glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &minAlignment);
                if (layout.GetStride() % minAlignment != 0)
                {
                    HE_ENGINE_LOG_CRITICAL("Uniform buffer layout must be a multiple of {0} but is {1}", minAlignment, layout.GetStride());
                    HE_ENGINE_ASSERT(false);
                }
            }
        }

        glBindBuffer(OpenGLCommon::BufferTypeToOpenGL(type), m_BufferId);
        glBufferData(OpenGLCommon::BufferTypeToOpenGL(type), layout.GetStride() * elementCount, initialData, OpenGLCommon::BufferUsageTypeToOpenGL(usage));
        glBindBuffer(OpenGLCommon::BufferTypeToOpenGL(type), 0);
    }

    OpenGLBuffer::~OpenGLBuffer()
    {
        glDeleteBuffers(1, &m_BufferId);
    }

    void OpenGLBuffer::SetBytes(void* data, u32 byteCount, u32 byteOffset)
    {
        HE_PROFILE_FUNCTION();
        
        if (m_Usage == BufferUsageType::Static)
        {
            HE_ENGINE_LOG_WARN("Attemting to update buffer that is marked as static");
            return;
        }

        glBindBuffer(OpenGLCommon::BufferTypeToOpenGL(m_Type), m_BufferId);
        glBufferSubData(OpenGLCommon::BufferTypeToOpenGL(m_Type), byteOffset, byteCount, data);
        glBindBuffer(OpenGLCommon::BufferTypeToOpenGL(m_Type), 0);
    }
}