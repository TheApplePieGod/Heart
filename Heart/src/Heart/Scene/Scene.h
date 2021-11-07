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
        void DestroyEntity(Entity entity);

        template<typename Component>
        void ClearComponent()
        {
            m_Registry.clear<Component>();
        }

        void ClearScene();

        entt::registry& GetRegistry() { return m_Registry; }

    private:
        entt::registry m_Registry;
    };
}