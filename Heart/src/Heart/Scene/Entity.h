#pragma once

#include "Heart/Core/UUID.h"
#include "entt/entt.hpp"

namespace Heart
{
    class Scene;
    class Entity
    {
    public:
        Entity(Scene* scene, entt::entity handle);
        Entity(Scene* scene, u32 handle);
        Entity() = default;

        template<typename Component>
        bool HasComponent()
        {
            return m_Scene->GetRegistry().all_of<Component>(m_EntityHandle);
        }

        // Will replace existing component of same type
        template<typename Component, typename ... Args>
        Component& AddComponent(Args&& ... args)
        {
            return m_Scene->GetRegistry().emplace_or_replace<Component>(m_EntityHandle, std::forward<Args>(args)...);
        }

        // Safe to call when entity does not have component
        template<typename Component>
        void RemoveComponent()
        {
            m_Scene->GetRegistry().remove<Component>(m_EntityHandle);
        }

        template<typename Component>
        Component& GetComponent()
        {
            HE_ENGINE_ASSERT(HasComponent<Component>(), "Cannot get, entity does not have component");
            return m_Scene->GetRegistry().get<Component>(m_EntityHandle);
        }

        UUID GetUUID();
        bool IsValid();
        void Destroy();

        inline Scene* GetScene() { return m_Scene; }
        inline entt::entity GetHandle() { return m_EntityHandle; }

    private:
        entt::entity m_EntityHandle = entt::null;
        Scene* m_Scene = nullptr;
    };
}