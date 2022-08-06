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

        u32 allocCount = Heart::Container<u32>::MinimumAllocCount + 1;
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

        u32 allocCount = Heart::Container<u32>::MinimumAllocCount + 1;
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
        Heart::HVector<DataStruct> vTest2 = {
            DataStruct(1),
            DataStruct(2),
            DataStruct(3)
        };
        DataStructsAllocated = 0;

        void* oldData = vTest2.Data();
        vTest2.Remove(1);

        CHECK(vTest2.GetCount() == 2);
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(vTest2.Data() == oldData);
        CHECK(vTest2.Get(0).Value == 1);
        CHECK(vTest2.Get(1).Value == 3);
        CHECK(vTest2.Get(1).GetRefCount() == 1);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 1);

        vTest2.Remove(0);
        vTest2.Remove(0);

        CHECK(vTest2.GetCount() == 0);
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(vTest2.Data() == oldData);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 3);

        CHECK_THROWS_AS(vTest2.Remove(0), std::out_of_range);
    }
    SUBCASE("RemoveUnordered")
    {
        Heart::HVector<DataStruct> vTest2 = {
            DataStruct(1),
            DataStruct(2),
            DataStruct(3),
            DataStruct(4)
        };
        DataStructsAllocated = 0;

        void* oldData = vTest2.Data();
        vTest2.RemoveUnordered(1);

        CHECK(vTest2.GetCount() == 3);
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(vTest2.Data() == oldData);
        CHECK(vTest2.Get(0).Value == 1);
        CHECK(vTest2.Get(1).Value == 4);
        CHECK(vTest2.Get(1).GetRefCount() == 1);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 1);

        vTest2.RemoveUnordered(0);
        vTest2.RemoveUnordered(0);
        vTest2.RemoveUnordered(0);

        CHECK(vTest2.GetCount() == 0);
        CHECK(vTest2.GetRefCount() == 1);
        CHECK(vTest2.Data() == oldData);
        CHECK(DataStructsAllocated == 0);
        CHECK(DataStructsDeallocated == 4);

        CHECK_THROWS_AS(vTest2.RemoveUnordered(0), std::out_of_range);
    }
}