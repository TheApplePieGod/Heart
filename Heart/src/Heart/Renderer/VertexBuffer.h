#pragma once

#include "Heart/Renderer/Buffer.h"

namespace Heart
{
    class VertexBuffer : public Buffer
    {
    public:
        VertexBuffer(const BufferLayout& layout, u32 elementCount)
            : Buffer(layout, elementCount)
        {}
        virtual ~VertexBuffer() = default;

    public:
        static Ref<VertexBuffer> Create(const BufferLayout& layout, u32 elementCount);
        static Ref<VertexBuffer> Create(const BufferLayout& layout, u32 elementCount, void* initialData);

    private:

    };
}