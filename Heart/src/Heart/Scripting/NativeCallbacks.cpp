#include "hepch.h"

#include "Heart/Core/Log.h"
#include "Heart/Input/Input.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Container/HArray.h"
#include "Heart/Container/HString.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Asset/AssetManager.h"

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
    if (!Heart::ScriptingEngine::IsScriptInputEnabled()) return false;
    return Heart::Input::IsKeyPressed(key);
}

HE_INTEROP_EXPORT bool Native_Input_IsMouseButtonPressed(Heart::MouseCode button)
{
    if (!Heart::ScriptingEngine::IsScriptInputEnabled()) return false;
    return Heart::Input::IsMouseButtonPressed(button);
}

/*
 * Scene Functions
 */

HE_INTEROP_EXPORT void Native_Scene_CreateEntity(Heart::Scene* sceneHandle, const char* name, u32* entityHandle)
{
    *entityHandle = (u32)sceneHandle->CreateEntity(name).GetHandle();
}

HE_INTEROP_EXPORT void Native_Scene_GetEntityFromUUID(Heart::Scene* sceneHandle, Heart::UUID uuid, u32* entityHandle)
{
    *entityHandle = (u32)sceneHandle->GetEntityFromUUID(uuid).GetHandle();
}

HE_INTEROP_EXPORT void Native_Scene_GetEntityFromName(Heart::Scene* sceneHandle, const char* name, u32* entityHandle)
{
    *entityHandle = (u32)sceneHandle->GetEntityFromName(name).GetHandle();
}

HE_INTEROP_EXPORT bool Native_Scene_RaycastSingle(Heart::Scene* sceneHandle, const Heart::RaycastInfo* info, Heart::RaycastResult* result)
{
    return sceneHandle->GetPhysicsWorld().RaycastSingle(*info, *result);
}

/*
 * Entity Functions
 */

HE_INTEROP_EXPORT void Native_Entity_Destroy(u32 entityHandle, Heart::Scene* sceneHandle)
{
    sceneHandle->DestroyEntity({ sceneHandle, entityHandle });
}

HE_INTEROP_EXPORT bool Native_Entity_IsValid(u32 entityHandle, Heart::Scene* sceneHandle)
{
    Heart::Entity entity(sceneHandle, entityHandle);
    return entity.IsValid();
}

/*
 * Asset manager functions
 */

HE_INTEROP_EXPORT void Native_AssetManager_GetAssetUUID(const char* path, bool isResource, Heart::UUID* outId)
{
    *outId = Heart::AssetManager::GetAssetUUID(path, isResource);
}

/*
 * Component Functions
 */

#ifdef HE_ENABLE_ASSERTS
    #define ASSERT_ENTITY_HAS_COMPONENT(compName) \
        { \
            Heart::Entity entity(sceneHandle, entityHandle); \
            HE_ENGINE_ASSERT( \
                entity.HasComponent<Heart::compName>(), \
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
    #define ASSERT_ENTITY_HAS_COMPONENT(compName)
    #define ASSERT_ENTITY_IS_VALID()
#endif

#define EXPORT_COMPONENT_GET_FN(compName) \
    HE_INTEROP_EXPORT void Native_##compName##_Get(u32 entityHandle, Heart::Scene* sceneHandle, Heart::compName** outComp) \
    { \
        ASSERT_ENTITY_IS_VALID(); \
        ASSERT_ENTITY_HAS_COMPONENT(compName); \
        Heart::Entity entity(sceneHandle, entityHandle); \
        *outComp = &entity.GetComponent<Heart::compName>(); \
    }

#define EXPORT_COMPONENT_EXISTS_FN(compName) \
    HE_INTEROP_EXPORT bool Native_##compName##_Exists(u32 entityHandle, Heart::Scene* sceneHandle) \
    { \
        ASSERT_ENTITY_IS_VALID(); \
        Heart::Entity entity(sceneHandle, entityHandle); \
        return entity.HasComponent<Heart::compName>(); \
    } \

#define EXPORT_COMPONENT_ADD_FN(compName) \
    HE_INTEROP_EXPORT void Native_##compName##_Add(u32 entityHandle, Heart::Scene* sceneHandle) \
    { \
        ASSERT_ENTITY_IS_VALID(); \
        Heart::Entity entity(sceneHandle, entityHandle); \
        entity.AddComponent<Heart::compName>(); \
    } \

#define EXPORT_COMPONENT_REMOVE_FN(compName) \
    HE_INTEROP_EXPORT void Native_##compName##_Remove(u32 entityHandle, Heart::Scene* sceneHandle) \
    { \
        ASSERT_ENTITY_IS_VALID(); \
        Heart::Entity entity(sceneHandle, entityHandle); \
        entity.RemoveComponent<Heart::compName>(); \
    } \

#define EXPORT_COMPONENT_BASIC_FNS(compName) \
    EXPORT_COMPONENT_GET_FN(compName) \
    EXPORT_COMPONENT_EXISTS_FN(compName) \
    EXPORT_COMPONENT_ADD_FN(compName) \
    EXPORT_COMPONENT_REMOVE_FN(compName)

// Id component (always exists)
EXPORT_COMPONENT_GET_FN(IdComponent);

// Name component (always exists)
EXPORT_COMPONENT_GET_FN(NameComponent);

HE_INTEROP_EXPORT void Native_NameComponent_SetName(u32 entityHandle, Heart::Scene* sceneHandle, Heart::HString value)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.GetComponent<Heart::NameComponent>().Name = value.Convert(Heart::HString::Encoding::UTF8);
}

// Transform component (always exists)
EXPORT_COMPONENT_GET_FN(TransformComponent);

HE_INTEROP_EXPORT void Native_TransformComponent_SetPosition(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3 pos)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetPosition(pos);
}

HE_INTEROP_EXPORT void Native_TransformComponent_SetRotation(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3 rot)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetRotation(rot);
}

HE_INTEROP_EXPORT void Native_TransformComponent_SetScale(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3 scale)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetScale(scale);
}

HE_INTEROP_EXPORT void Native_TransformComponent_SetTransform(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetTransform(pos, rot, scale);
}

HE_INTEROP_EXPORT void Native_TransformComponent_ApplyRotation(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3 rot)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.ApplyRotation(rot);
}

// TODO: we eventually want to move this logic to c# (probably) (or something)
HE_INTEROP_EXPORT void Native_TransformComponent_GetForwardVector(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3* outValue)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    *outValue = entity.GetForwardVector();
}

// Parent component
HE_INTEROP_EXPORT void Native_ParentComponent_Get(u32 entityHandle, Heart::Scene* sceneHandle, Heart::UUID* outComp)
{ 
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    *outComp = entity.GetParent();
}

HE_INTEROP_EXPORT void Native_ParentComponent_SetParent(u32 entityHandle, Heart::Scene* sceneHandle, Heart::UUID parent)
{ 
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetParent(parent);
}

// Children component
HE_INTEROP_EXPORT void Native_ChildrenComponent_Get(u32 entityHandle, Heart::Scene* sceneHandle, Heart::UUID** outComp)
{ 
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    *outComp = entity.GetChildren().Data();
}

HE_INTEROP_EXPORT void Native_ChildrenComponent_AddChild(u32 entityHandle, Heart::Scene* sceneHandle, Heart::UUID uuid)
{ 
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.AddChild(uuid);
}

HE_INTEROP_EXPORT void Native_ChildrenComponent_RemoveChild(u32 entityHandle, Heart::Scene* sceneHandle, Heart::UUID uuid)
{ 
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.RemoveChild(uuid);
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

HE_INTEROP_EXPORT void Native_ScriptComponent_SetScriptClass(u32 entityHandle, Heart::Scene* sceneHandle, const char* value)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(ScriptComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.GetComponent<Heart::ScriptComponent>().Instance.SetScriptClass(value);
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
    instance.OnConstruct();
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

// Primary camera component
EXPORT_COMPONENT_EXISTS_FN(PrimaryCameraComponent);

// Camera component
EXPORT_COMPONENT_BASIC_FNS(CameraComponent);

HE_INTEROP_EXPORT void Native_CameraComponent_SetPrimary(u32 entityHandle, Heart::Scene* sceneHandle, bool primary)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(CameraComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetIsPrimaryCameraEntity(primary);
}

// Rigid body component
EXPORT_COMPONENT_GET_FN(CollisionComponent);
EXPORT_COMPONENT_EXISTS_FN(CollisionComponent);
EXPORT_COMPONENT_REMOVE_FN(CollisionComponent);

HE_INTEROP_EXPORT void Native_CollisionComponent_Add(u32 entityHandle, Heart::Scene* sceneHandle)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    auto body = Heart::PhysicsBody::CreateDefaultBody((void*)(intptr_t)entity.GetUUID());
    entity.AddComponent<Heart::CollisionComponent>(body);
}

HE_INTEROP_EXPORT void Native_CollisionComponent_GetInfo(u32 entityHandle, Heart::Scene* sceneHandle, Heart::PhysicsBodyCreateInfo* outValue)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(CollisionComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    *outValue = entity.GetPhysicsBody()->GetInfo();
}

HE_INTEROP_EXPORT void Native_CollisionComponent_GetShapeType(u32 entityHandle, Heart::Scene* sceneHandle, u32* outValue)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(CollisionComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    *outValue = (u32)entity.GetPhysicsBody()->GetShapeType();
}

HE_INTEROP_EXPORT void Native_CollisionComponent_UpdateType(u32 entityHandle, Heart::Scene* sceneHandle, Heart::PhysicsBodyType type)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(CollisionComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    auto info = entity.GetPhysicsBody()->GetInfo();
    info.Type = type;
    entity.ReplacePhysicsBody(entity.GetPhysicsBody()->Clone(&info));
}

HE_INTEROP_EXPORT void Native_CollisionComponent_UpdateMass(u32 entityHandle, Heart::Scene* sceneHandle, float mass)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(CollisionComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    auto info = entity.GetPhysicsBody()->GetInfo();
    info.Mass = mass;
    entity.ReplacePhysicsBody(entity.GetPhysicsBody()->Clone(&info));
}

HE_INTEROP_EXPORT void Native_CollisionComponent_UpdateCollisionChannels(u32 entityHandle, Heart::Scene* sceneHandle, u64 channels)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(CollisionComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    auto info = entity.GetPhysicsBody()->GetInfo();
    info.CollisionChannels = channels;
    entity.ReplacePhysicsBody(entity.GetPhysicsBody()->Clone(&info));
}

HE_INTEROP_EXPORT void Native_CollisionComponent_UpdateCollisionMask(u32 entityHandle, Heart::Scene* sceneHandle, u64 mask)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(CollisionComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    auto info = entity.GetPhysicsBody()->GetInfo();
    info.CollisionMask = mask;
    entity.ReplacePhysicsBody(entity.GetPhysicsBody()->Clone(&info));
}

HE_INTEROP_EXPORT void Native_CollisionComponent_UseBoxShape(u32 entityHandle, Heart::Scene* sceneHandle, const Heart::PhysicsBodyCreateInfo* info, glm::vec3 extent)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(CollisionComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.ReplacePhysicsBody(Heart::PhysicsBody::CreateBoxShape(*info, extent));
}

HE_INTEROP_EXPORT void Native_CollisionComponent_UseSphereShape(u32 entityHandle, Heart::Scene* sceneHandle, const Heart::PhysicsBodyCreateInfo* info, float radius)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(CollisionComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.ReplacePhysicsBody(Heart::PhysicsBody::CreateSphereShape(*info, radius));
}

HE_INTEROP_EXPORT void Native_CollisionComponent_UseCapsuleShape(u32 entityHandle, Heart::Scene* sceneHandle, const Heart::PhysicsBodyCreateInfo* info, float radius, float halfHeight)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(CollisionComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.ReplacePhysicsBody(Heart::PhysicsBody::CreateCapsuleShape(*info, radius, halfHeight));
}

// Text component
EXPORT_COMPONENT_BASIC_FNS(TextComponent);

HE_INTEROP_EXPORT void Native_TextComponent_SetText(u32 entityHandle, Heart::Scene* sceneHandle, const char* text)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(TextComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetText(text);
}

HE_INTEROP_EXPORT void Native_TextComponent_ClearRenderData(u32 entityHandle, Heart::Scene* sceneHandle)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(TextComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.GetComponent<Heart::TextComponent>().ClearRenderData();
}

// We need this in order to ensure that the dllexports inside the engine static lib
// do not get removed
void* exportVariable = nullptr;
