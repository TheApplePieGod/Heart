#include "hepch.h"
#include "Entity.h"

#include "Heart/Container/HString.h"
#include "Heart/Container/Variant.h"

namespace Heart
{
    template<>
    void Entity::AddComponent<CollisionComponent>(PhysicsBody& body)
    {
        if (HasComponent<CollisionComponent>())
            m_Scene->GetPhysicsWorld().RemoveBody(GetComponent<CollisionComponent>().BodyId);
        body.SetTransform(GetWorldPosition(), m_Scene->GetEntityCachedQuat(*this));
        u32 id = m_Scene->GetPhysicsWorld().AddBody(body);
        m_Scene->GetRegistry().emplace_or_replace<CollisionComponent>(m_EntityHandle, id);
    }

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
        return m_Scene->m_CachedTransforms[m_EntityHandle].Transform;
    }

    glm::vec3 Entity::GetWorldPosition()
    {
        return m_Scene->m_CachedTransforms[m_EntityHandle].Position;
    }

    glm::vec3 Entity::GetWorldRotation()
    {
        return m_Scene->m_CachedTransforms[m_EntityHandle].Rotation;
    }

    glm::vec3 Entity::GetWorldScale()
    {
        return m_Scene->m_CachedTransforms[m_EntityHandle].Scale;
    }

    glm::vec3 Entity::GetWorldForwardVector()
    {
        return glm::normalize(glm::vec3(glm::toMat4(glm::quat(glm::radians(m_Scene->m_CachedTransforms[m_EntityHandle].Rotation))) * glm::vec4(0.f, 0.f, 1.f, 1.f)));
    }

    void Entity::SetPosition(glm::vec3 pos)
    {
        GetComponent<TransformComponent>().Translation = pos;
        m_Scene->CacheEntityTransform(*this);
    }

    void Entity::SetRotation(glm::vec3 rot)
    {
        GetComponent<TransformComponent>().Rotation = rot;
        m_Scene->CacheEntityTransform(*this);
    }

    void Entity::SetScale(glm::vec3 scale)
    {
        GetComponent<TransformComponent>().Scale = scale;
        m_Scene->CacheEntityTransform(*this);
    }

    void Entity::SetTransform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
    {
        auto& comp = GetComponent<TransformComponent>();
        comp.Translation = pos;
        comp.Rotation = rot;
        comp.Scale = scale;
        m_Scene->CacheEntityTransform(*this);
    }

    void Entity::ApplyRotation(glm::vec3 rot)
    {
        auto& comp = GetComponent<TransformComponent>();
        auto newRot = glm::quat(glm::radians(rot)) * comp.GetRotationQuat();
        comp.Rotation = glm::degrees(glm::eulerAngles(newRot));
        m_Scene->CacheEntityTransform(*this);
    }

    const HVector<UUID>& Entity::GetChildren()
    {
        if (!HasComponent<ChildrenComponent>())
            AddComponent<ChildrenComponent>();
        return GetComponent<ChildrenComponent>().Children;
    }

    void Entity::AddChild(UUID uuid)
    {
        Entity child = m_Scene->GetEntityFromUUID(uuid);
        if (!child.IsValid()) return;
        m_Scene->AssignRelationship(*this, child);
    }

    void Entity::RemoveChild(UUID uuid)
    {
        Entity child = m_Scene->GetEntityFromUUID(uuid);
        if (!child.IsValid()) return;
        m_Scene->UnparentEntity(child);
    }

    UUID Entity::GetParent() const
    {
        if (!HasComponent<ParentComponent>())
            return 0;
        return GetComponent<ParentComponent>().ParentUUID;
    }

    void Entity::SetParent(UUID uuid)
    {
        Entity parent = m_Scene->GetEntityFromUUID(uuid);
        if (!parent.IsValid()) return;
        m_Scene->AssignRelationship(parent, *this);
    }

    Variant Entity::GetScriptProperty(const HStringView8& name) const
    {
        auto& comp = GetComponent<ScriptComponent>();
        return comp.Instance.GetFieldValue(name);
    }

    void Entity::SetScriptProperty(const HStringView8& name, const Variant& value)
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
}
