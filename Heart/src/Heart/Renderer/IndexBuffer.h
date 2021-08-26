#pragma once

#include "Heart/Renderer/Buffer.h"

namespace Heart
{
    class IndexBuffer : public Buffer
    {
    public:
        IndexBuffer(u32 indexCount)
            : Buffer(indexCount)
        {
            m_Layout = {
                { BufferDataType::UInt }
            };
        }
        
        virtual ~IndexBuffer() = default;

    public:
        static Ref<IndexBuffer> Create(u32 indexCount);
        static Ref<IndexBuffer> Create(u32 indexCount, u32* initialData);

    private:

    };
}