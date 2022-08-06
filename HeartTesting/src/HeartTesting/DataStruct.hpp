#pragma once

#include <ostream>

inline static int DataStructsAllocated = 0;
inline static int DataStructsDeallocated = 0;

struct DataStruct
{
    DataStruct()
    {
        Allocate();
    }

    DataStruct(u32 value)
        : Value(value)
    {
        Allocate();
    }

    DataStruct(const DataStruct& other)
    {
        *this = other;
    }

    ~DataStruct()
    {
        Cleanup();
    }

    void Allocate()
    {
        RefCount = new u32(1);
        DataStructsAllocated++;
    }

    void Cleanup()
    {
        if (!RefCount) return;
        if (--*RefCount == 0)
        {
            delete RefCount;
            DataStructsDeallocated++;
        }
        RefCount = nullptr;
    }

    u32 GetRefCount() const
    {
        if (!RefCount) return 1;
        return *RefCount;
    }

    void operator=(const DataStruct& other)
    {
        Cleanup();
        RefCount = other.RefCount;
        Value = other.Value;
        if (RefCount) ++*RefCount;
    }

    bool operator==(const DataStruct& other) const
    {
        return RefCount == other.RefCount &&
               Value == other.Value;
    }

    u32* RefCount = nullptr;
    u32 Value = 0;
};

std::ostream& operator<<(std::ostream& os, const DataStruct& value)
{
    os << "{ RefCount: " << value.GetRefCount() << ", Value: " << value.Value << " }";
    return os;
}