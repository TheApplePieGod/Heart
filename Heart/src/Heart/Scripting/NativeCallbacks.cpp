#include "hepch.h"

#include "Heart/Core/Log.h"
#include "Heart/Input/Input.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Container/HArray.h"
#include "Heart/Container/HString.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Task/JobManager.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Scripting/ManagedIterator.h"
#include "Heart/Scripting/EntityView.h"

#define HE_INTEROP_EXPORT extern "C" [[maybe_unused]]

// https://stackoverflow.com/questions/56097222/keywords-in-out-ref-vs-attributes-in-out-in-out
// https://docs.microsoft.com/en-us/dotnet/standard/native-interop/best-practices

HE_INTEROP_EXPORT void Native_Log(int level, const char16* message, u32 messageLen)
{
    auto converted = Heart::HStringView16(message, messageLen).ToUTF8();
    Heart::Logger::GetClientLogger().log(spdlog::source_loc{}, static_cast<spdlog::level::level_enum>(level), converted.Data());
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

HE_INTEROP_EXPORT bool Native_Input_IsButtonPressed(Heart::ButtonCode button)
{
    if (!Heart::ScriptingEngine::IsScriptInputEnabled()) return false;
    return Heart::Input::IsButtonPressed(button);
}

HE_INTEROP_EXPORT bool Native_Input_GetAxisValue(Heart::AxisCode axis)
{
    if (!Heart::ScriptingEngine::IsScriptInputEnabled()) return false;
    return Heart::Input::GetAxisValue(axis);
}

HE_INTEROP_EXPORT bool Native_Input_GetAxisDelta(Heart::AxisCode axis)
{
    if (!Heart::ScriptingEngine::IsScriptInputEnabled()) return false;
    return Heart::Input::GetAxisDelta(axis);
}

/*
 * Scene Functions
 */

HE_INTEROP_EXPORT void Native_Scene_CreateEntity(Heart::Scene* sceneHandle, const char16* name, u32 nameLen, u32* entityHandle)
{
    auto converted = Heart::HStringView16(name, nameLen).ToUTF8();
    *entityHandle = (u32)sceneHandle->CreateEntity(converted, false).GetHandle();
}

HE_INTEROP_EXPORT void Native_Scene_GetEntityFromUUID(Heart::Scene* sceneHandle, Heart::UUID uuid, u32* entityHandle)
{
    *entityHandle = (u32)sceneHandle->GetEntityFromUUID(uuid).GetHandle();
}

HE_INTEROP_EXPORT void Native_Scene_GetEntityFromName(Heart::Scene* sceneHandle, const char16* name, u32 nameLen, u32* entityHandle)
{
    auto converted = Heart::HStringView16(name, nameLen).ToUTF8();
    *entityHandle = (u32)sceneHandle->GetEntityFromName(converted).GetHandle();
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

HE_INTEROP_EXPORT void Native_AssetManager_GetAssetUUID(const char16* path, u32 pathLen, bool isResource, Heart::UUID* outId)
{
    auto converted = Heart::HStringView16(path, pathLen).ToUTF8();
    *outId = Heart::AssetManager::GetAssetUUID(converted, isResource);
}

/*
 * Entity view functions
 */
HE_INTEROP_EXPORT void Native_EntityView_Init(void** outView, Heart::Scene* sceneHandle)
{
    auto viewHandle = new Heart::EntityView();
    viewHandle->View.iterate(sceneHandle->GetRegistry().storage<Heart::TransformComponent>());
    viewHandle->Current = viewHandle->View.begin();

    *outView = viewHandle;
}

HE_INTEROP_EXPORT void Native_EntityView_Destroy(void* view)
{
    delete ((Heart::EntityView*)view);
}

HE_INTEROP_EXPORT bool Native_EntityView_GetNext(void* _view, u32* outEntityHandle)
{
    auto view = (Heart::EntityView*)_view;
    if (view->Current == view->View.end())
        return false;
    *outEntityHandle = (u32)*(view->Current++);
    return true;
}

/*
 * Task functions
 */

using Native_SchedulableIter_RunFn = void (*)(size_t);
HE_INTEROP_EXPORT void Native_SchedulableIter_Schedule(
    Heart::ManagedIterator<size_t>::GetNextIterFn getNext,
    Native_SchedulableIter_RunFn runFunc)
{
    Heart::ManagedIterator<size_t> begin(getNext);
    Heart::ManagedIterator<size_t> end(nullptr);

    Heart::Job handle = Heart::JobManager::ScheduleIter(begin, end, runFunc);
    handle.Wait();
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
    #define ASSERT_ENTITY_HAS_RUNTIME_COMPONENT(id) \
        { \
            Heart::Entity entity(sceneHandle, entityHandle); \
            HE_ENGINE_ASSERT( \
                entity.HasRuntimeComponent(id), \
                "Entity HasRuntimeComponent check failed" \
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
    #define ASSERT_ENTITY_HAS_RUNTIME_COMPONENT(id)
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

#define EXPORT_COMPONENT_GET_FN_UNCHECKED(compName) \
    HE_INTEROP_EXPORT void Native_##compName##_Get(u32 entityHandle, Heart::Scene* sceneHandle, Heart::compName** outComp) \
    { \
        ASSERT_ENTITY_IS_VALID(); \
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
EXPORT_COMPONENT_GET_FN_UNCHECKED(IdComponent);

// Name component (always exists)
EXPORT_COMPONENT_GET_FN_UNCHECKED(NameComponent);

HE_INTEROP_EXPORT void Native_NameComponent_SetName(u32 entityHandle, Heart::Scene* sceneHandle, Heart::HString value)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.GetComponent<Heart::NameComponent>().Name = value.Convert(Heart::HString::Encoding::UTF8);
}

// Transform component (always exists)
EXPORT_COMPONENT_GET_FN_UNCHECKED(TransformComponent);

HE_INTEROP_EXPORT void Native_TransformComponent_SetPosition(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3 pos)
{
    HE_PROFILE_FUNCTION();
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetPosition(pos, false);
}

HE_INTEROP_EXPORT void Native_TransformComponent_SetRotation(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3 rot)
{
    HE_PROFILE_FUNCTION();
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetRotation(rot, false);
}

HE_INTEROP_EXPORT void Native_TransformComponent_SetScale(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3 scale)
{
    HE_PROFILE_FUNCTION();
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetScale(scale, false);
}

HE_INTEROP_EXPORT void Native_TransformComponent_SetTransform(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
{
    HE_PROFILE_FUNCTION();
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetTransform(pos, rot, scale, false);
}

HE_INTEROP_EXPORT void Native_TransformComponent_ApplyRotation(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3 rot)
{
    HE_PROFILE_FUNCTION();
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.ApplyRotation(rot, false);
}

// TODO: we eventually want to move this logic to c# (probably) (or something)
HE_INTEROP_EXPORT void Native_TransformComponent_GetForwardVector(u32 entityHandle, Heart::Scene* sceneHandle, glm::vec3* outValue)
{
    HE_PROFILE_FUNCTION();
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
    entity.SetParent(parent, false);
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
    HE_PROFILE_FUNCTION();
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.AddChild(uuid, false);
}

HE_INTEROP_EXPORT void Native_ChildrenComponent_RemoveChild(u32 entityHandle, Heart::Scene* sceneHandle, Heart::UUID uuid)
{ 
    HE_PROFILE_FUNCTION();
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.RemoveChild(uuid, false);
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
EXPORT_COMPONENT_EXISTS_FN(ScriptComponent);
EXPORT_COMPONENT_ADD_FN(ScriptComponent);
EXPORT_COMPONENT_REMOVE_FN(ScriptComponent);

HE_INTEROP_EXPORT void Native_ScriptComponent_GetObjectHandle(u32 entityHandle, Heart::Scene* sceneHandle, uptr* outValue)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(ScriptComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    *outValue = entity.GetComponent<Heart::ScriptComponent>().Instance.GetObjectHandle();
}

HE_INTEROP_EXPORT void Native_ScriptComponent_GetScriptClass(u32 entityHandle, Heart::Scene* sceneHandle, const Heart::HString** outValue)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(ScriptComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    *outValue = &entity.GetComponent<Heart::ScriptComponent>().Instance.GetScriptClassObject().GetFullName();
}

HE_INTEROP_EXPORT void Native_ScriptComponent_SetScriptClass(u32 entityHandle, Heart::Scene* sceneHandle, const char16* value, u32 valueLen)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(ScriptComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    Heart::HStringView view(value, valueLen);
    s64 typeId = Heart::ScriptingEngine::GetClassIdFromName(view.Convert(Heart::HString::Encoding::UTF8));
    entity.GetComponent<Heart::ScriptComponent>().Instance.SetScriptClassId(typeId);
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

HE_INTEROP_EXPORT void Native_TextComponent_SetText(u32 entityHandle, Heart::Scene* sceneHandle, const char16* text, u32 textLen)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(TextComponent);
    auto converted = Heart::HStringView16(text, textLen).ToUTF8();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.SetText(converted);
}

HE_INTEROP_EXPORT void Native_TextComponent_ClearRenderData(u32 entityHandle, Heart::Scene* sceneHandle)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_COMPONENT(TextComponent);
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.GetComponent<Heart::TextComponent>().ClearRenderData();
}

// Runtime components
// Special implementations required here 
HE_INTEROP_EXPORT void Native_RuntimeComponent_Get(u32 entityHandle, Heart::Scene* sceneHandle, s64 typeId, uptr* outComp)
{
    ASSERT_ENTITY_IS_VALID();
    ASSERT_ENTITY_HAS_RUNTIME_COMPONENT(typeId);
    Heart::Entity entity(sceneHandle, entityHandle);
    *outComp = entity.GetRuntimeComponent(typeId).Instance.GetObjectHandle();
}

HE_INTEROP_EXPORT bool Native_RuntimeComponent_Exists(u32 entityHandle, Heart::Scene* sceneHandle, s64 typeId)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    return entity.HasRuntimeComponent(typeId);
}

HE_INTEROP_EXPORT void Native_RuntimeComponent_Add(u32 entityHandle, Heart::Scene* sceneHandle, s64 typeId, uptr objectHandle)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.AddRuntimeComponent(typeId, objectHandle);
}

HE_INTEROP_EXPORT void Native_RuntimeComponent_Remove(u32 entityHandle, Heart::Scene* sceneHandle, s64 typeId)
{
    ASSERT_ENTITY_IS_VALID();
    Heart::Entity entity(sceneHandle, entityHandle);
    entity.RemoveRuntimeComponent(typeId);
}

// TODO: codegen? It's a bit complicated since the class names have to be in alphabetical order
void* NativeCallbacks[] = {
    (void*)&Native_AssetManager_GetAssetUUID,
    (void*)&Native_CameraComponent_Get,
    (void*)&Native_CameraComponent_Exists,
    (void*)&Native_CameraComponent_Add,
    (void*)&Native_CameraComponent_Remove,
    (void*)&Native_CameraComponent_SetPrimary,
    (void*)&Native_PrimaryCameraComponent_Exists,
    (void*)&Native_ChildrenComponent_Get,
    (void*)&Native_ChildrenComponent_AddChild,
    (void*)&Native_ChildrenComponent_RemoveChild,
    (void*)&Native_CollisionComponent_Get,
    (void*)&Native_CollisionComponent_Add,
    (void*)&Native_CollisionComponent_Remove,
    (void*)&Native_CollisionComponent_Exists,
    (void*)&Native_CollisionComponent_GetInfo,
    (void*)&Native_CollisionComponent_GetShapeType,
    (void*)&Native_CollisionComponent_UpdateType,
    (void*)&Native_CollisionComponent_UpdateMass,
    (void*)&Native_CollisionComponent_UpdateCollisionChannels,
    (void*)&Native_CollisionComponent_UpdateCollisionMask,
    (void*)&Native_CollisionComponent_UseBoxShape,
    (void*)&Native_CollisionComponent_UseSphereShape,
    (void*)&Native_CollisionComponent_UseCapsuleShape,
    (void*)&Native_Entity_Destroy,
    (void*)&Native_Entity_IsValid,
    (void*)&Native_EntityView_Init,
    (void*)&Native_EntityView_Destroy,
    (void*)&Native_EntityView_GetNext,
    (void*)&Native_HArray_Init,
    (void*)&Native_HArray_Destroy,
    (void*)&Native_HArray_Copy,
    (void*)&Native_HArray_Add,
    (void*)&Native_HString_Init,
    (void*)&Native_HString_Destroy,
    (void*)&Native_HString_Copy,
    (void*)&Native_IdComponent_Get,
    (void*)&Native_Input_IsKeyPressed,
    (void*)&Native_Input_IsButtonPressed,
    (void*)&Native_Input_GetAxisValue,
    (void*)&Native_Input_GetAxisDelta,
    (void*)&Native_LightComponent_Get,
    (void*)&Native_LightComponent_Add,
    (void*)&Native_LightComponent_Remove,
    (void*)&Native_LightComponent_Exists,
    (void*)&Native_Log,
    (void*)&Native_MeshComponent_Get,
    (void*)&Native_MeshComponent_Exists,
    (void*)&Native_MeshComponent_Add,
    (void*)&Native_MeshComponent_Remove,
    (void*)&Native_MeshComponent_AddMaterial,
    (void*)&Native_MeshComponent_RemoveMaterial,
    (void*)&Native_NameComponent_Get,
    (void*)&Native_NameComponent_SetName,
    (void*)&Native_ParentComponent_Get,
    (void*)&Native_ParentComponent_SetParent,
    (void*)&Native_RuntimeComponent_Exists,
    (void*)&Native_RuntimeComponent_Add,
    (void*)&Native_RuntimeComponent_Remove,
    (void*)&Native_RuntimeComponent_Get,
    (void*)&Native_Scene_CreateEntity,
    (void*)&Native_Scene_GetEntityFromUUID,
    (void*)&Native_Scene_GetEntityFromName,
    (void*)&Native_Scene_RaycastSingle,
    (void*)&Native_SchedulableIter_Schedule,
    (void*)&Native_ScriptComponent_Exists,
    (void*)&Native_ScriptComponent_Add,
    (void*)&Native_ScriptComponent_Remove,
    (void*)&Native_ScriptComponent_GetObjectHandle,
    (void*)&Native_ScriptComponent_GetScriptClass,
    (void*)&Native_ScriptComponent_SetScriptClass,
    (void*)&Native_ScriptComponent_InstantiateScript,
    (void*)&Native_ScriptComponent_DestroyScript,
    (void*)&Native_TextComponent_Get,
    (void*)&Native_TextComponent_Exists,
    (void*)&Native_TextComponent_Add,
    (void*)&Native_TextComponent_Remove,
    (void*)&Native_TextComponent_SetText,
    (void*)&Native_TextComponent_ClearRenderData,
    (void*)&Native_TransformComponent_Get,
    (void*)&Native_TransformComponent_SetPosition,
    (void*)&Native_TransformComponent_SetRotation,
    (void*)&Native_TransformComponent_SetScale,
    (void*)&Native_TransformComponent_SetTransform,
    (void*)&Native_TransformComponent_ApplyRotation,
    (void*)&Native_TransformComponent_GetForwardVector,
    (void*)&Native_Variant_Destroy,
    (void*)&Native_Variant_FromHArray,
    (void*)&Native_Variant_FromHString,
};
