#include "hepch.h"

#include "Heart/Core/Log.h"
#include "Heart/Input/Input.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Container/HArray.h"
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

/*
 * HString Functions
 */

HE_INTEROP_EXPORT void Native_HString_Init(Heart::HString* str, const char16* value)
{
    str->~HString();
    HE_PLACEMENT_NEW(str, Heart::HString, value);
}

HE_INTEROP_EXPORT void Native_HString_Destroy(Heart::HString* str)
{
    str->~HString();
}

HE_INTEROP_EXPORT void Native_HString_Copy(Heart::HString* dst, const Heart::HString* src)
{
    dst->~HString();
    HE_PLACEMENT_NEW(dst, Heart::HString, *src);
}

/*
 * HArray Functions
 */

HE_INTEROP_EXPORT void Native_HArray_Init(Heart::HArray* array)
{
    // Reserve base elems when initializing from c# so ptr is always valid
    array->~HArray();
    HE_PLACEMENT_NEW(array, Heart::HArray, 16, false);
}

HE_INTEROP_EXPORT void Native_HArray_Destroy(Heart::HArray* array)
{
    array->~HArray();
}

HE_INTEROP_EXPORT void Native_HArray_Copy(Heart::HArray* dst, const Heart::HArray* src)
{
    dst->~HArray();
    HE_PLACEMENT_NEW(dst, Heart::HArray, *src);
}

HE_INTEROP_EXPORT void Native_HArray_Add(Heart::HArray* array, const Heart::Variant* value)
{
    array->Add(*value);
}

/*
 * Variant Functions
 */

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

/*
 * Input Functions
 */

HE_INTEROP_EXPORT bool Native_Input_IsKeyPressed(Heart::KeyCode key)
{
    return Heart::Input::IsKeyPressed(key);
}

HE_INTEROP_EXPORT bool Native_Input_IsMouseButtonPressed(Heart::MouseCode button)
{
    return Heart::Input::IsMouseButtonPressed(button);
}

/*
 * Scene Functions
 */

HE_INTEROP_EXPORT void Native_Scene_CreateEntity(Heart::Scene* sceneHandle, Heart::HString name, u32* entityHandle)
{
    *entityHandle = (u32)sceneHandle->CreateEntity(name.ToUTF8()).GetHandle();
}

HE_INTEROP_EXPORT void Native_Scene_GetEntityFromUUID(Heart::Scene* sceneHandle, Heart::UUID uuid, u32* entityHandle)
{
    *entityHandle = (u32)sceneHandle->GetEntityFromUUID(uuid).GetHandle();
}

/*
 * Entity Functions
 */

HE_INTEROP_EXPORT void Native_Entity_Destroy(u32 entityHandle, Heart::Scene* sceneHandle)
{
    sceneHandle->DestroyEntity({ sceneHandle, entityHandle });
}

/*
 * Component Functions
 */

#ifdef HE_ENABLE_ASSERTS
    #define ASSERT_ENTITY_HAS_COMPONENT(compName) \
        { \
            Heart::Entity entity(sceneHandle, entityHandle); \
            HE_ENGINE_ASSERT( \
                entity.HasComponent<Heart::##compName>(), \
                "Entity HasComponent check failed for " #compName \
            ); \
        }
    #define ASSERT_ENTITY_IS_VALID() \
        { \
            Heart::Entity entity(sceneHandle, entityHandle); \
            HE_ENGINE_ASSERT( \
                entity.IsValid(), \
                "Entity IsValid check failed" \
            ); \
        }
#else
    #define ASSERT_ENTITY_HAS_COMPONENT()
    #define ASSERT_ENTITY_IS_VALID()
#endif

#define EXPORT_COMPONENT_GET_FN(compName) \
    HE_INTEROP_EXPORT void Native_##compName##_Get(u32 entityHandle, Heart::Scene* sceneHandle, Heart::##compName** outComp) \
    { \
        ASSERT_ENTITY_IS_VALID(); \
        ASSERT_ENTITY_HAS_COMPONENT(compName); \
        Heart::Entity entity(sceneHandle, entityHandle); \
        *outComp = &entity.GetComponent<Heart::##compName>(); \
    }

#define EXPORT_COMPONENT_EXISTS_FN(compName) \
    HE_INTEROP_EXPORT bool Native_##compName##_Exists(u32 entityHandle, Heart::Scene* sceneHandle) \
    { \
        ASSERT_ENTITY_IS_VALID(); \
        Heart::Entity entity(sceneHandle, entityHandle); \
        return entity.HasComponent<Heart::##compName>(); \
    } \

#define EXPORT_COMPONENT_BASIC_FNS(compName) \
    EXPORT_COMPONENT_GET_FN(compName) \
    EXPORT_COMPONENT_EXISTS_FN(compName)

// Id component (always exists)
EXPORT_COMPONENT_GET_FN(IdComponent);

// Name component (always exists)
EXPORT_COMPONENT_GET_FN(NameComponent);

HE_INTEROP_EXPORT void Native_NameComponent_SetName(u32 entityHandle, Heart::Scene* sceneHandle, Heart::HString value)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(NameComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.GetComponent<Heart::NameComponent>().Name = value.ToUTF8();
}

// Transform component (always exists)
EXPORT_COMPONENT_GET_FN(TransformComponent);

HE_INTEROP_EXPORT void Native_TransformComponent_CacheTransform(u32 entityHandle, Heart::Scene* sceneHandle)
{
    ASSERT_ENTITY_IS_VALID();
    sceneHandle->CacheEntityTransform({ sceneHandle, entityHandle });
}

// Mesh component
EXPORT_COMPONENT_BASIC_FNS(MeshComponent);

HE_INTEROP_EXPORT void Native_MeshComponent_AddMaterial(u32 entityHandle, Heart::Scene* sceneHandle, Heart::UUID material)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(MeshComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.GetComponent<Heart::MeshComponent>().Materials.AddInPlace(material);
}

HE_INTEROP_EXPORT void Native_MeshComponent_RemoveMaterial(u32 entityHandle, Heart::Scene* sceneHandle, u32 index)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(MeshComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.GetComponent<Heart::MeshComponent>().Materials.Remove(index);
}

// Light component
EXPORT_COMPONENT_BASIC_FNS(LightComponent);

// Script component
EXPORT_COMPONENT_BASIC_FNS(ScriptComponent);

HE_INTEROP_EXPORT void Native_ScriptComponent_SetScriptClass(u32 entityHandle, Heart::Scene* sceneHandle, Heart::HString value)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(ScriptComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.GetComponent<Heart::ScriptComponent>().Instance.SetScriptClass(value.ToUTF8());
}

HE_INTEROP_EXPORT void Native_ScriptComponent_InstantiateScript(u32 entityHandle, Heart::Scene* sceneHandle)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(ScriptComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    auto& instance = entity.GetComponent<Heart::ScriptComponent>().Instance;
    if (instance.IsAlive())
        instance.OnPlayEnd();
    instance.Instantiate({ sceneHandle, entityHandle });
    instance.OnPlayStart();
}

HE_INTEROP_EXPORT void Native_ScriptComponent_DestroyScript(u32 entityHandle, Heart::Scene* sceneHandle)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(ScriptComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    auto& instance = entity.GetComponent<Heart::ScriptComponent>().Instance;
    instance.OnPlayEnd();
    instance.Destroy();
}

// We need this in order to ensure that the dllexports inside the engine static lib
// do not get removed
void* exportVariable = nullptr;