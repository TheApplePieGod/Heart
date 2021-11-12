#include "htpch.h"
#include "Entity.h"

#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Components.h"

namespace Heart
{
    Entity::Entity(Scene* scene, entt::entity handle)
        : m_Scene(scene), m_EntityHandle(handle)
    {}

    Entity::Entity(Scene* scene, u32 handle)
        : m_Scene(scene), m_EntityHandle(static_cast<entt::entity>(handle))
    {}

    UUID Entity::GetUUID()
    {
        return GetComponent<IdComponent>().UUID;
    }

    bool Entity::IsValid()
    {
        return m_Scene && m_Scene->GetRegistry().valid(m_EntityHandle);
    }
}