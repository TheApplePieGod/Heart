#include "htpch.h"
#include "Entity.h"

#include "Heart/Scene/Scene.h"

namespace Heart
{
    Entity::Entity(Scene* scene, entt::entity handle)
        : m_Scene(scene), m_EntityHandle(handle)
    {}

    bool Entity::IsValid()
    {
        return m_Scene && m_Scene->GetRegistry().valid(m_EntityHandle);
    }
}