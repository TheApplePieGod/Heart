#include "htpch.h"
#include "Buffer.h"

namespace Heart
{
    u32 BufferLayout::CalculateStride()
    {
        u32 stride = 0;

        for (auto& element : m_Elements)
            stride += element.CalculatedSize;

        if (stride % 4 != 0)
            HE_ENGINE_LOG_WARN("Buffer layout of length {0} has stride {1} that is not four byte aligned", m_Elements.size(), stride);

        return stride;
    }
}