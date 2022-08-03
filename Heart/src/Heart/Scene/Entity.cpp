#include "hepch.h"
#include "Entity.h"

#include "Heart/Container/HString.h"
#include "Heart/Container/Variant.h"

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
        return m_Scene && m_Scene->m_Registry.valid(m_EntityHandle);
    }

    void Entity::Destroy()
    {
        m_Scene->DestroyEntity(*this);
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

    Variant Entity::GetScriptProperty(const HString& name) const
    {
        auto& comp = GetComponent<ScriptComponent>();
        return comp.Instance.GetFieldValue(name);
    }

    void Entity::SetScriptProperty(const HString& name, const Variant& value)
    {
        auto& comp = GetComponent<ScriptComponent>();
        comp.Instance.SetFieldValue(name, value);
    }
}