#pragma once

namespace Heart
{
    enum class BufferDataType
    {
        None = 0,
        Bool,
        UInt,
        Double,
        Int, Int2, Int3, Int4,
        Float, Float2, Float3, Float4,
        Mat3, Mat4  
    };

    // returns the size in bytes
    static u32 BufferDataTypeSize(BufferDataType type)
    {
        switch (type)
        {
            case BufferDataType::Bool: return 4;
            case BufferDataType::UInt: return 4;
            case BufferDataType::Double: return 8;
            case BufferDataType::Int: return 4;
            case BufferDataType::Int2: return 4 * 2;
            case BufferDataType::Int3: return 4 * 3;
            case BufferDataType::Int4: return 4 * 4;
            case BufferDataType::Float: return 4;
            case BufferDataType::Float2: return 4 * 2;
            case BufferDataType::Float3: return 4 * 3;
            case BufferDataType::Float4: return 4 * 4;
            case BufferDataType::Mat3: return 4 * 3 * 3;
            case BufferDataType::Mat4: return 4 * 4 * 4;
        }

        HE_ENGINE_ASSERT(false, "BufferDataTypeSize unsupported BufferDataType");
        return 0;
    }

    struct BufferLayoutElement
    {
        BufferLayoutElement(u32 size) // for padding fields
            : CalculatedSize(size)
        {}
        BufferLayoutElement(BufferDataType type)
            : DataType(type), CalculatedSize(BufferDataTypeSize(type))
        {}

        BufferDataType DataType;
        u32 CalculatedSize;
    };

    class BufferLayout
    {
    public:
        BufferLayout() = default;
        BufferLayout(std::initializer_list<BufferLayoutElement> elements)
            : m_Elements(elements)
        {
            m_CalculatedStride = CalculateStride();
        }

        inline u32 GetStride() const { return m_CalculatedStride; }
        inline std::vector<BufferLayoutElement>& GetElements() { return m_Elements; }
    
    private:
        u32 CalculateStride();

    private:
        u32 m_CalculatedStride;
        std::vector<BufferLayoutElement> m_Elements;
    };

    class Buffer
    {
    public:
        Buffer(const BufferLayout& layout, u32 elementCount)
            : m_Layout(layout), m_AllocatedCount(elementCount)
        {}
        Buffer(u32 elementCount)
            : m_AllocatedCount(elementCount)
        {}

        virtual void SetData(void* data, u32 elementCount, u32 elementOffset) = 0;

        inline BufferLayout& GetLayout() { return m_Layout; }
        inline u32 GetAllocatedSize() const { return m_AllocatedCount * m_Layout.GetStride(); }
        inline u32 GetAllocatedCount() const { return m_AllocatedCount; }

    protected:
        BufferLayout m_Layout;
        u32 m_AllocatedCount;
    };
}