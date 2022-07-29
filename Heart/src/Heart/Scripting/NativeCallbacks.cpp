#include "hepch.h"

#include "Heart/Core/Log.h"
#include "Heart/Container/HArray.h"

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

HE_INTEROP_EXPORT void Native_HArray_Init(Heart::HArray* array)
{
    // Placement new so destructor doesn't get called
    HE_PLACEMENT_NEW(array, Heart::HArray);
}

HE_INTEROP_EXPORT void Native_HArray_Destroy(Heart::HArray* array)
{
    array->~HArray();
}

HE_INTEROP_EXPORT void Native_HArray_Add(Heart::HArray* array, Heart::Variant* value)
{
    array->Add(*value);
}

HE_INTEROP_EXPORT void Native_Variant_FromHArray(Heart::Variant* variant, Heart::HArray* value)
{
    // Placement new so destructor doesn't get called
    HE_PLACEMENT_NEW(variant, Heart::Variant, *value);
}

HE_INTEROP_EXPORT void Native_Variant_Destroy(Heart::Variant* variant)
{
    variant->~Variant();
}

// We need this in order to ensure that the dllexports inside the engine static lib
// do not get removed
void* exportVariable = nullptr;