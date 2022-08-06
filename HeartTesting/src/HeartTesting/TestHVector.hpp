#pragma once
#include "doctest/doctest.h"

#include "Heart/Container/HVector.hpp"
#include "HeartTesting/DataStruct.hpp"

TEST_CASE("Testing HVector")
{
    Heart::HVector<DataStruct> vData = {
        DataStruct(1),
        DataStruct(2),
        DataStruct(3)
    };
    Heart::HVector<DataStruct> vTest;

    DataStructsAllocated = 0;
    DataStructsDeallocated = 0;

    REQUIRE(vData.GetCount() == 3);
    REQUIRE(vData.GetAllocatedCount() >= 0);
    REQUIRE(vTest.Data() == nullptr);
    REQUIRE(vTest.IsEmpty());
    REQUIRE(vTest.GetCount() == 0);
    REQUIRE(vTest.GetAllocatedCount() == 0);

    SUBCASE("Copy constructor")
    {
        Heart::HVector<DataStruct> vTest2(vData);

        CHECK(vTest2.GetCount() == vData.GetCount());
        CHECK(vTest2.Data() != vData.Data()); // ensure copy
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(vTest2.Front() == vData.Front());
        CHECK(vTest2.Back() == vData.Back());
        CHECK(vTest2.Front().GetRefCount() == 2);
        CHECK(DataStructsAllocated == 0);
    }
    SUBCASE("ElemCount constructor (fill = false)")
    {
        Heart::HVector<DataStruct> vTest2(3, false);

        CHECK(vTest2.GetCount() == 0);
        CHECK(vTest2.GetAllocatedCount() > 0);
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(DataStructsAllocated == 0);
    }
    SUBCASE("ElemCount constructor (fill = true)")
    {
        Heart::HVector<DataStruct> vTest2(3, true);

        CHECK(vTest2.GetCount() == 3);
        CHECK(vTest2.GetAllocatedCount() > 0);
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(vTest2.Front().GetRefCount() == 1);
        CHECK(vTest2.Back().GetRefCount() == 1);
        CHECK(DataStructsAllocated == 3);
    }
    SUBCASE("Data constructor")
    {
        Heart::HVector<DataStruct> vTest2(vData.Data(), vData.GetCount());

        CHECK(vTest2.GetCount() == 3);
        CHECK(vTest2.Data() != vData.Data()); // ensure copy
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(vTest2.Front() == vData.Front());
        CHECK(vTest2.Back() == vData.Back());
        CHECK(vTest2.Front().GetRefCount() == 2);
        CHECK(DataStructsAllocated == 0);
    }
    SUBCASE("Initializer list constructor")
    {
        std::initializer_list<DataStruct> list = {
            DataStruct(1),
            DataStruct(2),
            DataStruct(3)
        };
        Heart::HVector<DataStruct> vTest2(list);

        CHECK(vTest2.GetCount() == 3);
        CHECK(vTest2.Data() != list.begin()); // ensure copy
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(vTest2.Front() == *list.begin());
        CHECK(vTest2.Back() == *(list.end() - 1));
        CHECK(vTest2.Front().GetRefCount() == 2);
        CHECK(DataStructsAllocated == list.size());
    }
    SUBCASE("Start & end data constructor")
    {
        Heart::HVector<DataStruct> vTest2(vData.Begin(), vData.End());

        CHECK(vTest2.GetCount() == 3);
        CHECK(vTest2.Data() != vData.begin()); // ensure copy
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(vTest2.Front() == vData.Front());
        CHECK(vTest2.Back() == vData.Back());
        CHECK(vTest2.Front().GetRefCount() == 2);
        CHECK(DataStructsAllocated == 0);
    }
    SUBCASE("Add")
    {
        vTest.Add(DataStruct(1));

        void* oldData = vTest.Data();
        CHECK(vTest.GetCount() == 1);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.Front().Value == 1);
        CHECK(vTest.Back().Value == 1);
        CHECK(vTest.Front().GetRefCount() == 1);
        CHECK(DataStructsAllocated == 1);

        vTest.Add(DataStruct(2));

        CHECK(vTest.GetCount() == 2);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.Data() == oldData);
        CHECK(vTest.Front().Value == 1);
        CHECK(vTest.Back().Value == 2);
        CHECK(vTest.Front().GetRefCount() == 1);
        CHECK(DataStructsAllocated == 2);

        u32 allocCount = Heart::Container<DataStruct>::MinimumAllocCount + 1;
        while (vTest.GetCount() < allocCount)
            vTest.Add(DataStruct(3));

        CHECK(vTest.GetCount() == allocCount);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.Data() != oldData); // data moved
        CHECK(vTest.Front().Value == 1);
        CHECK(vTest.Back().Value == 3);
        CHECK(vTest.Front().GetRefCount() == 1);
        CHECK(DataStructsAllocated == allocCount);
    }
    SUBCASE("Add in-place")
    {
        vTest.AddInPlace(1);

        void* oldData = vTest.Data();
        CHECK(vTest.GetCount() == 1);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.Front().Value == 1);
        CHECK(vTest.Back().Value == 1);
        CHECK(vTest.Front().GetRefCount() == 1);
        CHECK(DataStructsAllocated == 1);

        vTest.AddInPlace(2);

        CHECK(vTest.GetCount() == 2);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.Data() == oldData);
        CHECK(vTest.Front().Value == 1);
        CHECK(vTest.Back().Value == 2);
        CHECK(vTest.Front().GetRefCount() == 1);
        CHECK(DataStructsAllocated == 2);

        u32 allocCount = Heart::Container<DataStruct>::MinimumAllocCount + 1;
        while (vTest.GetCount() < allocCount)
            vTest.AddInPlace(3);

        CHECK(vTest.GetCount() == allocCount);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.Data() != oldData); // data moved
        CHECK(vTest.Front().Value == 1);
        CHECK(vTest.Back().Value == 3);
        CHECK(vTest.Front().GetRefCount() == 1);
        CHECK(DataStructsAllocated == allocCount);
    }
    SUBCASE("Remove")
    {
        void* oldData = vData.Data();
        vData.Remove(1);

        CHECK(vData.GetCount() == 2);
        CHECK(vData.GetRefCount() == 1);
        CHECK(vData.Data() == oldData);
        CHECK(vData.Get(0).Value == 1);
        CHECK(vData.Get(1).Value == 3);
        CHECK(vData.Get(1).GetRefCount() == 1);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 1);

        vData.Remove(0);
        vData.Remove(0);

        CHECK(vData.GetCount() == 0);
        CHECK(vData.GetRefCount() == 1);
        CHECK(vData.Data() == oldData);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 3);

        CHECK_THROWS_AS(vData.Remove(0), std::out_of_range);
    }
    SUBCASE("Remove unordered")
    {
        vData.Add(DataStruct(4));
        DataStructsAllocated = 0;

        void* oldData = vData.Data();
        vData.RemoveUnordered(1);

        CHECK(vData.GetCount() == 3);
        CHECK(vData.GetRefCount() == 1);
        CHECK(vData.Data() == oldData);
        CHECK(vData.Get(0).Value == 1);
        CHECK(vData.Get(1).Value == 4);
        CHECK(vData.Get(1).GetRefCount() == 1);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 1);

        vData.RemoveUnordered(0);
        vData.RemoveUnordered(0);
        vData.RemoveUnordered(0);

        CHECK(vData.GetCount() == 0);
        CHECK(vData.GetRefCount() == 1);
        CHECK(vData.Data() == oldData);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 4);

        CHECK_THROWS_AS(vData.RemoveUnordered(0), std::out_of_range);
    }
    SUBCASE("Pop")
    {
        void* oldData = vData.Data();
        vData.Pop();

        CHECK(vData.GetCount() == 2);
        CHECK(vData.GetRefCount() == 1);
        CHECK(vData.Data() == oldData);
        CHECK(vData.Get(0).Value == 1);
        CHECK(vData.Get(0).GetRefCount() == 1);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 1);

        vData.Pop();
        vData.Pop();

        CHECK(vData.GetCount() == 0);
        CHECK(vData.GetRefCount() == 1);
        CHECK(vData.Data() == oldData);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 3);

        CHECK_THROWS_AS(vData.Pop(), std::out_of_range);
    }
    SUBCASE("Copy from")
    {
        vTest.CopyFrom(vData.Begin(), vData.End());

        CHECK(vTest.GetCount() == vData.GetCount());
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.Front() == vData.Front());
        CHECK(vTest.Back() == vData.Back());
        CHECK(vTest.Front().GetRefCount() == 2);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 0);
    }
    SUBCASE("Append (shallow = false)")
    {
        Heart::HVector<DataStruct> vTest2 = {
            DataStruct(1),
            DataStruct(2),
            DataStruct(3)
        };
        DataStructsAllocated = 0;

        void* oldData = vTest2.Data();
        vTest2.Append(vData, false);

        CHECK(vTest2.GetCount() == vData.GetCount() + 3);
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(vTest2.Data() == oldData);
        CHECK(vTest2.Front().Value == 1);
        CHECK(vTest2.Back() == vData.Back());
        CHECK(vTest2.Front().GetRefCount() == 1);
        CHECK(vTest2.Back().GetRefCount() == 2);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 0);
    }
    SUBCASE("Append (shallow = true)")
    {
        Heart::HVector<DataStruct> vTest2 = {
            DataStruct(1),
            DataStruct(2),
            DataStruct(3)
        };
        DataStructsAllocated = 0;

        void* oldData = vTest2.Data();
        vTest2.Append(vData, true);

        CHECK(vTest2.GetCount() == vData.GetCount() + 3);
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(vTest2.Data() == oldData);
        CHECK(vTest2.Front().Value == 1);
        CHECK(vTest2.Back() == vData.Back());
        CHECK(vTest2.Front().GetRefCount() == 1);
        CHECK(vTest2.Back().GetRefCount() == 1);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 0);
    }
    SUBCASE("Reserve")
    {
        u32 allocCount = Heart::Container<DataStruct>::MinimumAllocCount;
        vTest.Reserve(allocCount);

        CHECK(vTest.GetCount() == 0);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.GetAllocatedCount() == allocCount);
        CHECK(DataStructsAllocated == 0);

        void* oldData = vTest.Data();
        vTest.Reserve(allocCount - 1);

        CHECK(vTest.GetCount() == 0);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.GetAllocatedCount() == allocCount);
        CHECK(vTest.Data() == oldData);
        CHECK(DataStructsAllocated == 0);
    }
    SUBCASE("Clear (shrink = false)")
    {
        u32 oldSize = vData.GetCount();
        u32 oldAllocated = vData.GetAllocatedCount();
        void* oldData = vData.Data();
        vData.Clear(false);

        CHECK(vData.GetCount() == 0);
        CHECK(vData.GetRefCount() == 1);
        CHECK(vData.GetAllocatedCount() == oldAllocated);
        CHECK(vData.Data() == oldData);
        CHECK(DataStructsDeallocated == oldSize);
    }
    SUBCASE("Clear (shrink = true)")
    {
        u32 oldSize = vData.GetCount();
        vData.Clear(true);

        CHECK(vData.GetCount() == 0);
        CHECK(vData.GetRefCount() == 1);
        CHECK(vData.GetAllocatedCount() == 0);
        CHECK(vData.Data() == nullptr);
        CHECK(DataStructsDeallocated == oldSize);
    }
    SUBCASE("Resize (construct = false)")
    {
        vTest.Resize(3, false);

        CHECK(vTest.GetCount() == 3);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.GetAllocatedCount() >= 3);
        CHECK(DataStructsAllocated == 0);
        
        // Not constructing is dangerous because it assumes that the user
        // will handle construction. If that doesn't happen, then garbage
        // data will get destructed when the vector goes out of scope which
        // will cause a crash in this scenario
        memset(vTest.Data(), 0, vTest.GetCount() * sizeof(DataStruct));
    }
    SUBCASE("Resize (construct = true)")
    {
        u32 allocCount = Heart::Container<DataStruct>::MinimumAllocCount;
        vTest.Resize(allocCount, true);

        CHECK(vTest.GetCount() == allocCount);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.GetAllocatedCount() == allocCount);
        CHECK(vTest.Front().GetRefCount() == 1);
        CHECK(DataStructsAllocated == allocCount);

        void* oldData = vTest.Data();
        vTest.Resize(1, true);

        CHECK(vTest.GetCount() == 1);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.GetAllocatedCount() == allocCount);
        CHECK(vTest.Front().GetRefCount() == 1);
        CHECK(vTest.Data() == oldData);
        CHECK(DataStructsAllocated == allocCount);
        CHECK(DataStructsDeallocated == allocCount - 1);

        vTest.Resize(0, true);

        CHECK(vTest.GetCount() == 0);
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vTest.GetAllocatedCount() == 0);
        CHECK(vTest.Data() == nullptr);
        CHECK(DataStructsAllocated == allocCount);
        CHECK(DataStructsDeallocated == allocCount);
    }
    SUBCASE("Clone")
    {
        vTest = vData.Clone();

        CHECK(vTest.GetCount() == vData.GetCount());
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vData.GetRefCount() == 1);
        CHECK(vTest.Front() == vData.Front());
        CHECK(vTest.Back() == vData.Back());
        CHECK(vTest.Front().GetRefCount() == 2);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 0);
    }
    SUBCASE("Shallow copy")
    {
        vTest.ShallowCopy(vData);

        CHECK(vTest.GetCount() == vData.GetCount());
        CHECK(vTest.GetRefCount() == 2);
        CHECK(vTest.Data() == vData.Data());
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 0);
    }
    SUBCASE("Assignment operator")
    {
        vTest = vData;

        CHECK(vTest.GetCount() == vData.GetCount());
        CHECK(vTest.GetRefCount() == 1);
        CHECK(vData.GetRefCount() == 1);
        CHECK(vTest.Front() == vData.Front());
        CHECK(vTest.Back() == vData.Back());
        CHECK(vTest.Front().GetRefCount() == 2);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 0);
    }
}