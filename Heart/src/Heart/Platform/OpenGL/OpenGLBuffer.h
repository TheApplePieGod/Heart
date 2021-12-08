#pragma once

#include "Heart/Renderer/Buffer.h"

namespace Heart
{    
    class OpenGLBuffer : public Buffer
    {
    public:
        OpenGLBuffer(Type type, BufferUsageType usage, const BufferLayout& layout, u32 elementCount, void* initialData);
        ~OpenGLBuffer() override;

        void SetBytes(void* data, u32 byteCount, u32 byteOffset) override;

        inline u32 GetBufferId() const { return m_BufferId; }

    private:
        u32 m_BufferId;
    };
}