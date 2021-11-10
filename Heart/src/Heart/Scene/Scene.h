#pragma once

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
        Entity DuplicateEntity(Entity source);
        void DestroyEntity(Entity entity);

        template<typename Component>
        void ClearComponent()
        {
            m_Registry.clear<Component>();
        }

        void ClearScene();

        entt::registry& GetRegistry() { return m_Registry; }

    private:
        template<typename Component>
        void CopyComponent(entt::entity src, entt::entity dst)
        {
            if (m_Registry.any_of<Component>(src))
                m_Registry.emplace<Component>(dst, m_Registry.get<Component>(src));
        }

    private:
        entt::registry m_Registry;
    };
}