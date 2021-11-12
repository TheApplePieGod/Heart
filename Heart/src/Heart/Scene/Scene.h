#pragma once

#include "Heart/Core/UUID.h"
#include "entt/entt.hpp"

namespace Heart
{
    class Entity;
    class Scene
    {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity(const std::string& name);
        Entity CreateEntityWithUUID(const std::string& name, UUID uuid);
        Entity DuplicateEntity(Entity source);
        void DestroyEntity(Entity entity);
        void AssignRelationship(Entity parent, Entity child);
        void UnparentEntity(Entity child);

        template<typename Component>
        void ClearComponent()
        {
            m_Registry.clear<Component>();
        }

        void ClearScene();

        entt::registry& GetRegistry() { return m_Registry; }
        Entity GetEntityFromUUID(UUID uuid);

    private:
        template<typename Component>
        void CopyComponent(entt::entity src, entt::entity dst)
        {
            if (m_Registry.any_of<Component>(src))
                m_Registry.emplace<Component>(dst, m_Registry.get<Component>(src));
        }

        void RemoveChild(entt::entity parent, UUID childUUID);

    private:
        entt::registry m_Registry;
        std::unordered_map<UUID, entt::entity> m_UUIDMap;
    };
}