#include "hepch.h"
#include "Entity.h"

#include "Heart/Scene/Scene.h"

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
        m_Scene->m_CachedTransforms.erase(m_EntityHandle);
        m_Scene->m_Registry.destroy(m_EntityHandle);
    }

    glm::mat4x4 Entity::GetWorldTransformMatrix()
    {
        return m_Scene->m_CachedTransforms[m_EntityHandle];
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
}