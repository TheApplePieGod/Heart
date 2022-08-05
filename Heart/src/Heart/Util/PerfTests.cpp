#include "hepch.h"
#include "PerfTests.h"

#include "Heart/Core/Timing.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HArray.h"
#include "Heart/Container/HString.h"

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
        HString test1 = "123";
        HString test2 = "123";
        bool test = test1 == test2;

        HString utf8 = "\xE0\xA4\xAF\xE0\xA5\x82\xE0\xA4\xA8\xE0\xA4\xBF\xE0\xA4\x95\xE0\xA5\x8B\xE0\xA4\xA1";
        HE_ENGINE_LOG_WARN(utf8.DataUTF8());
        HString utf16 = utf8.ToUTF16();
        utf8 = utf16.ToUTF8();
        HE_ENGINE_LOG_WARN(utf16.DataUTF8());
        HE_ENGINE_LOG_WARN(sizeof(HString));
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
        HVector<TestStruct> vec = { TestStruct(), TestStruct(), TestStruct(), TestStruct() };
        vec.Pop();
        int e = 0;

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

        int d = 0;
    }
}