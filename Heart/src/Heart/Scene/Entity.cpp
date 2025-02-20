#include "hepch.h"
#include "Entity.h"

#include "Heart/Container/HString.h"
#include "Heart/Container/Variant.h"
#include "Heart/Scripting/ScriptingEngine.h"

namespace Heart
{
    Entity::Entity(Scene* scene, entt::entity handle)
        : m_Scene(scene), m_EntityHandle(handle)
    {}

    Entity::Entity(Scene* scene, u32 handle)
        : m_Scene(scene), m_EntityHandle(static_cast<entt::entity>(handle))
    {}

    bool Entity::IsValid()
    {
        return m_Scene && m_Scene->m_Registry.valid(m_EntityHandle) && !HasComponent<DestroyedComponent>();
    }

    void Entity::Destroy()
    {
        m_Scene->DestroyEntity(*this);
    }

    void Entity::SetName(HStringView8 name)
    {
        GetComponent<NameComponent>().Name = name;
    }

    const glm::mat4x4& Entity::GetWorldTransformMatrix()
    {
        if (GetComponent<TransformComponent>().Dirty)
            m_Scene->CacheEntityTransform(*this);
        return m_Scene->m_CachedTransforms[m_EntityHandle].Transform;
    }

    glm::vec3 Entity::GetWorldPosition()
    {
        if (GetComponent<TransformComponent>().Dirty)
            m_Scene->CacheEntityTransform(*this);
        return m_Scene->m_CachedTransforms[m_EntityHandle].Position;
    }

    glm::vec3 Entity::GetWorldRotation()
    {
        if (GetComponent<TransformComponent>().Dirty)
            m_Scene->CacheEntityTransform(*this);
        return m_Scene->m_CachedTransforms[m_EntityHandle].Rotation;
    }

    glm::vec3 Entity::GetWorldScale()
    {
        if (GetComponent<TransformComponent>().Dirty)
            m_Scene->CacheEntityTransform(*this);
        return m_Scene->m_CachedTransforms[m_EntityHandle].Scale;
    }

    glm::vec3 Entity::GetWorldForwardVector()
    {
        if (GetComponent<TransformComponent>().Dirty)
            m_Scene->CacheEntityTransform(*this);
        return glm::normalize(glm::vec3(glm::toMat4(glm::quat(glm::radians(m_Scene->m_CachedTransforms[m_EntityHandle].Rotation))) * glm::vec4(0.f, 0.f, 1.f, 1.f)));
    }

    void Entity::SetPosition(glm::vec3 pos, bool cache)
    {
        auto& comp = GetComponent<TransformComponent>();
        comp.Translation = pos;
        if (cache)
            m_Scene->CacheEntityTransform(*this);
        else
            comp.Dirty = true;
    }

    void Entity::SetRotation(glm::vec3 rot, bool cache)
    {
        auto& comp = GetComponent<TransformComponent>();
        comp.Rotation = rot;
        if (cache)
            m_Scene->CacheEntityTransform(*this);
        else
            comp.Dirty = true;
    }

    void Entity::SetScale(glm::vec3 scale, bool cache)
    {
        auto& comp = GetComponent<TransformComponent>();
        comp.Scale = scale;
        if (cache)
            m_Scene->CacheEntityTransform(*this);
        else
            comp.Dirty = true;
    }

    void Entity::SetTransform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, bool cache)
    {
        auto& comp = GetComponent<TransformComponent>();
        comp.Translation = pos;
        comp.Rotation = rot;
        comp.Scale = scale;
        if (cache)
            m_Scene->CacheEntityTransform(*this);
        else
            comp.Dirty = true;
    }

    void Entity::ApplyRotation(glm::vec3 rot, bool cache)
    {
        auto& comp = GetComponent<TransformComponent>();
        auto newRot = glm::quat(glm::radians(rot)) * comp.GetRotationQuat();
        comp.Rotation = glm::degrees(glm::eulerAngles(newRot));
        if (cache)
            m_Scene->CacheEntityTransform(*this);
        else
            comp.Dirty = true;
    }

    bool Entity::HasChildren()
    {
        return HasComponent<ChildrenComponent>() && GetComponent<ChildrenComponent>().Children.Count() > 0;
    }

    const HVector<UUID>& Entity::GetChildren()
    {
        if (!HasComponent<ChildrenComponent>())
            AddComponent<ChildrenComponent>();
        return GetComponent<ChildrenComponent>().Children;
    }

    void Entity::AddChild(UUID uuid, bool cache)
    {
        Entity child = m_Scene->GetEntityFromUUID(uuid);
        if (!child.IsValid()) return;
        m_Scene->AssignRelationship(*this, child, cache);
    }

    void Entity::RemoveChild(UUID uuid, bool cache)
    {
        Entity child = m_Scene->GetEntityFromUUID(uuid);
        if (!child.IsValid()) return;
        m_Scene->UnparentEntity(child, cache);
    }

    UUID Entity::GetParent() const
    {
        if (!HasComponent<ParentComponent>())
            return 0;
        return GetComponent<ParentComponent>().ParentUUID;
    }

    void Entity::SetParent(UUID uuid, bool cache)
    {
        Entity parent = m_Scene->GetEntityFromUUID(uuid);
        if (!parent.IsValid()) return;
        m_Scene->AssignRelationship(parent, *this, cache);
    }

    Variant Entity::GetScriptProperty(const HString& name) const
    {
        auto& comp = GetComponent<ScriptComponent>();
        return comp.Instance.GetFieldValue(name);
    }

    void Entity::SetScriptProperty(const HString& name, const Variant& value)
    {
        auto& comp = GetComponent<ScriptComponent>();
        comp.Instance.SetFieldValue(name, value, true);
    }

    void Entity::SetIsPrimaryCameraEntity(bool primary)
    {
        if (primary)
        {
            auto prevPrimary = m_Scene->GetPrimaryCameraEntity();
            if (prevPrimary.IsValid())
                prevPrimary.RemoveComponent<PrimaryCameraComponent>();
            AddComponent<PrimaryCameraComponent>();
        }
        else if (HasComponent<PrimaryCameraComponent>())
            RemoveComponent<PrimaryCameraComponent>();
    }

    PhysicsBody* Entity::GetPhysicsBody()
    {
        auto& comp = GetComponent<CollisionComponent>();
        return m_Scene->GetPhysicsWorld().GetBody(comp.BodyId);
    }

    void Entity::ReplacePhysicsBody(const PhysicsBody& body, bool keepVel)
    {
        auto& comp = GetComponent<CollisionComponent>();
        m_Scene->GetPhysicsWorld().ReplaceBody(comp.BodyId, body, keepVel);        
    }

    void Entity::SetText(HStringView8 text)
    {
        auto& comp = GetComponent<TextComponent>();
        comp.Text = text;
        comp.ClearRenderData();
    }

    RuntimeComponent& Entity::GetRuntimeComponent(s64 typeId) const
    {
        HE_ENGINE_ASSERT(HasRuntimeComponent(typeId), "Cannot get, entity does not have specified runtime component");
        return m_Scene->GetRegistry().storage<RuntimeComponent>(typeId).get(m_EntityHandle);
    }

    bool Entity::HasRuntimeComponent(s64 typeId) const
    {
        // Special handling for runtime components. We need to get their unique storage associated
        // with their type id. However, we need to ensure we dont accidentally create it
        // if it doesn't exist, otherwise this could cause a race
        auto storage = m_Scene->GetRegistry().storage(typeId);
        if (!storage) return false;
        return storage->contains(m_EntityHandle);
    }

    void Entity::AddRuntimeComponent(s64 typeId, uptr objectHandle)
    {
        ScriptComponentInstance* instance;
        if (HasRuntimeComponent(typeId))
        {
            instance = &m_Scene->GetRegistry().storage<RuntimeComponent>(typeId).get(m_EntityHandle).Instance;
            instance->SetScriptClassId(typeId);
        }
        else
            instance = &m_Scene->GetRegistry().storage<RuntimeComponent>(typeId).emplace(m_EntityHandle, typeId).Instance;

        if (objectHandle == 0)
            instance->Instantiate();
        else
            instance->ConsumeHandle(objectHandle);
    }

    void Entity::RemoveRuntimeComponent(s64 typeId)
    {
        m_Scene->GetRegistry().storage<RuntimeComponent>(typeId).remove(m_EntityHandle);
    }
}
