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

    static u32 BufferDataTypeComponents(BufferDataType type)
    {
        switch (type)
        {
            case BufferDataType::Bool: return 1;
            case BufferDataType::UInt: return 1;
            case BufferDataType::Double: return 1;
            case BufferDataType::Int: return 4;
            case BufferDataType::Int2: return 2;
            case BufferDataType::Int3: return 3;
            case BufferDataType::Int4: return 4;
            case BufferDataType::Float: return 1;
            case BufferDataType::Float2: return 2;
            case BufferDataType::Float3: return 3;
            case BufferDataType::Float4: return 4;
            case BufferDataType::Mat3: return 3;
            case BufferDataType::Mat4: return 4;
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
        u32 Offset;
    };

    class BufferLayout
    {
    public:
        BufferLayout() = default;
        BufferLayout(std::initializer_list<BufferLayoutElement> elements)
            : m_Elements(elements)
        {
            m_CalculatedStride = CalculateStrideAndOffsets();
        }

        inline u32 GetStride() const { return m_CalculatedStride; }
        inline const std::vector<BufferLayoutElement>& GetElements() const { return m_Elements; }
    
    private:
        u32 CalculateStrideAndOffsets();

    private:
        u32 m_CalculatedStride;
        std::vector<BufferLayoutElement> m_Elements;
    };

    class Buffer // effectively a uniformbuffer
    {
    public:
        enum class Type
        {
            None = 0,
            Uniform, Storage, Vertex, Index
        };
        inline static const char* TypeStrings[] = {
            "None", "Uniform", "Storage", "Vertex", "Index"
        };

    public:
        Buffer(Type type, const BufferLayout& layout, u32 elementCount)
            : m_Type(type), m_Layout(layout), m_AllocatedCount(elementCount)
        {}
        virtual ~Buffer() = default;

        virtual void SetData(void* data, u32 elementCount, u32 elementOffset) = 0;

        inline Type GetType() const { return m_Type; }
        inline BufferLayout& GetLayout() { return m_Layout; }
        inline u32 GetAllocatedSize() const { return m_AllocatedCount * m_Layout.GetStride(); }
        inline u32 GetAllocatedCount() const { return m_AllocatedCount; }

    public:
        static Ref<Buffer> Create(Type type, const BufferLayout& layout, u32 elementCount);
        static Ref<Buffer> Create(Type type, const BufferLayout& layout, u32 elementCount, void* initialData);

        // added for convenience
        static Ref<Buffer> CreateIndexBuffer(u32 elementCount);
        static Ref<Buffer> CreateIndexBuffer(u32 elementCount, void* initialData);

    protected:
        BufferLayout m_Layout;
        u32 m_AllocatedCount;
        Type m_Type;
    };
}