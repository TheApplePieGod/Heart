#include "hepch.h"

#include "Heart/Core/Log.h"
#include "Heart/Container/HArray.hpp"
#include "Heart/Container/HString.h"

#define HE_INTEROP_EXPORT_BASE extern "C" [[maybe_unused]]
#ifdef HE_PLATFORM_WINDOWS
    #define HE_INTEROP_EXPORT HE_INTEROP_EXPORT_BASE __declspec(dllexport)
#elif defined(HE_PLATFORM_LINUX)
    #define HE_INTEROP_EXPORT HE_INTEROP_EXPORT_BASE __attribute__((visibility("default")))
#else
    #define HE_INTEROP_EXPORT HE_INTEROP_EXPORT_BASE
#endif

// https://stackoverflow.com/questions/56097222/keywords-in-out-ref-vs-attributes-in-out-in-out
// https://docs.microsoft.com/en-us/dotnet/standard/native-interop/best-practices

HE_INTEROP_EXPORT void Native_Log(int level, const char* message)
{
    Heart::Logger::GetClientLogger().log(spdlog::source_loc{}, static_cast<spdlog::level::level_enum>(level), message);
}

HE_INTEROP_EXPORT void Native_HString_Init(Heart::HString* str, const char16* value)
{
    HE_PLACEMENT_NEW(str, Heart::HString, value);
}

HE_INTEROP_EXPORT void Native_HString_Destroy(Heart::HString* str)
{
    str->~HString();
}

HE_INTEROP_EXPORT void Native_HString_Copy(Heart::HString* dst, const Heart::HString* src)
{
    HE_PLACEMENT_NEW(dst, Heart::HString, *src);
}

HE_INTEROP_EXPORT void Native_HArray_Init(Heart::HArray* array)
{
    HE_PLACEMENT_NEW(array, Heart::HArray);
}

HE_INTEROP_EXPORT void Native_HArray_Destroy(Heart::HArray* array)
{
    array->~HArray();
}

HE_INTEROP_EXPORT void Native_HArray_Copy(Heart::HArray* dst, const Heart::HArray* src)
{
    HE_PLACEMENT_NEW(dst, Heart::HArray, *src);
}

HE_INTEROP_EXPORT void Native_HArray_Add(Heart::HArray* array, const Heart::Variant* value)
{
    array->Add(*value);
}

HE_INTEROP_EXPORT void Native_Variant_FromHArray(Heart::Variant* variant, const Heart::HArray* value)
{
    HE_PLACEMENT_NEW(variant, Heart::Variant, *value);
}

HE_INTEROP_EXPORT void Native_Variant_FromHString(Heart::Variant* variant, const Heart::HString* value)
{
    HE_PLACEMENT_NEW(variant, Heart::Variant, *value);
}

HE_INTEROP_EXPORT void Native_Variant_Destroy(Heart::Variant* variant)
{
    variant->~Variant();
}

// We need this in order to ensure that the dllexports inside the engine static lib
// do not get removed
void* exportVariable = nullptr;