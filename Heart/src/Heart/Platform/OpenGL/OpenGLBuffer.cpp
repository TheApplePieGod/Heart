#include "htpch.h"
#include "OpenGLBuffer.h"

namespace Heart
{
    OpenGLBuffer::OpenGLBuffer(Type type, const BufferLayout& layout, u32 elementCount, void* initialData)
        : Buffer(type, layout, elementCount)
    {

    }

    OpenGLBuffer::~OpenGLBuffer()
    {

    }

    void OpenGLBuffer::SetData(void* data, u32 elementCount, u32 elementOffset)
    {

    }
}