#include "hepch.h"
#include "PerfTests.h"

#include "Heart/Core/Timing.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HArray.h"
#include "Heart/Container/HString.h"
#include "Heart/Container/HStringTyped.hpp"
#include "Heart/Container/HString8.h"

namespace Heart
{
    struct TestStruct
    {
        TestStruct() = default;
        ~TestStruct()
        {
            Field3 = false;
        }

        u64 Field1 = 12345;
        f32 Field2 = 12345.f;
        bool Field3 = true;
        u64 Field4 = 24680;
        HVector<u32> Field5;

        inline bool operator==(const TestStruct& other)
        {
            return Field1 == other.Field1 &&
                   Field2 == other.Field2 &&
                   Field3 == other.Field3 &&
                   Field4 == other.Field4;
        }
    };

    void PerfTests::RunHStringTest()
    {
        HString8 str1 = "asd";
        HStringView8 view1 = str1;

        HStringViewTyped<char8> view2 = view1;

        int d = 0;
    }

    void PerfTests::RunHArrayTest()
    {
        /*
         * HArray - Add String
         */
        {
            HArray arr;
            Timer timer = Timer("HArray - Add Strings");
            for (u32 i = 0; i < 1000000; i++)
                arr.Add(HString("brejiment"));
        }

        /*
         * HArray - Add Ints
         */
        {
            HArray arr;
            Timer timer = Timer("HArray - Add Integers");
            for (u32 i = 0; i < 1000000; i++)
                arr.Add(12345);
        }

        /*
         * HArray - Add Nested Array
         */
        {
            HArray arr;
            Timer timer = Timer("HArray - Add Nested Arrays");
            for (u32 i = 0; i < 1000000; i++)
                arr.Add(HArray(16, false)); // C# parity
        }
    }

    void PerfTests::RunHVectorTest()
    {
        TestStruct struct1;
        struct1.Field5.Add(5);
        TestStruct struct2 = struct1;
        struct1.Field5.Add(6);

        HVector<u32> testVec1 = { 1, 2, 3 };
        HVector<u32> testVec2 = { 4, 5, 6 };
        testVec1.Append(testVec2);

        HString8 testStr1 = "";
        HString8 testStr2 = "asd";
        HString8 testStr3;
        testStr2 = testStr1;
        int d = 0;

        /*
         * HVector - Add
         */
        {
            HVector<TestStruct> vec;
            Timer timer = Timer("HVector - Add");
            for (u32 i = 0; i < 1000000; i++)
                vec.AddInPlace();
            // for (u32 i = 0; i < 1000000; i++)
            //     HE_ENGINE_ASSERT(vec[i] == TestStruct());
        }

        /*
         * std::vector - Add
         */
        {
            std::vector<TestStruct> vec;
            Timer timer = Timer("std::vector - Add");
            for (u32 i = 0; i < 1000000; i++)
                vec.emplace_back();
            // for (u32 i = 0; i < 1000000; i++)
            //     HE_ENGINE_ASSERT(vec[i] == TestStruct());
        }

        /*
         * HVector - Remove
         */
        {
            HVector<TestStruct> vec(50000);
            Timer timer = Timer("HVector - Remove");
            for (u32 i = 0; i < 50000; i++)
                vec.Remove(0);
        }

        /*
         * std::vector - Remove
         */
        {
            std::vector<TestStruct> vec(50000);
            Timer timer = Timer("std::vector - Remove");
            for (u32 i = 0; i < 50000; i++)
                vec.erase(vec.begin());
        }

        /*
         * HVector - Add & Remove
         */
        {
            HVector<TestStruct> vec;
            Timer timer = Timer("HVector - Add & Remove");
            for (u32 j = 0; j < 25; j++)
            {
                for (u32 i = 0; i < 2500; i++)
                    vec.AddInPlace();
                
                for (u32 i = 0; i < 2500; i++)
                    vec.Remove(0);
            }
        }

        /*
         * std::vector - Add & Remove
         */
        {
            std::vector<TestStruct> vec;
            Timer timer = Timer("std::vector - Add & Remove");
            for (u32 j = 0; j < 25; j++)
            {
                for (u32 i = 0; i < 2500; i++)
                    vec.emplace_back();
                
                for (u32 i = 0; i < 2500; i++)
                    vec.erase(vec.begin());
            }
        }

        int g = 0;
    }
}