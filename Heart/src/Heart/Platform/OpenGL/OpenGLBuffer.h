#pragma once

#include "Heart/Renderer/Buffer.h"

namespace Heart
{    
    class OpenGLBuffer : public Buffer
    {
    public:
        OpenGLBuffer(Type type, const BufferLayout& layout, u32 elementCount, void* initialData);
        ~OpenGLBuffer() override;

        void SetData(void* data, u32 elementCount, u32 elementOffset) override;
    };
}