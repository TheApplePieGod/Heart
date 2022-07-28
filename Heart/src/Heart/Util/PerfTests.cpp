#include "hepch.h"
#include "PerfTests.h"

#include "Heart/Core/Timing.h"
#include "Heart/Container/HVector.hpp"

namespace Heart
{
    struct TestStruct
    {
        TestStruct() = default;

        ~TestStruct()
        {
            int d = 0;
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

    void PerfTests::RunHVectorTest()
    {
        // HVector<u64> vec = { 0, 1, 2, 3, 4, 5 };
        // vec.Remove(2);
        // int e = 0;

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